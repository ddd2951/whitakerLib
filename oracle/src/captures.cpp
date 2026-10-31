#include "lines.hpp"

namespace lines {

std::string readFile(const fs::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in)
    throw std::runtime_error("cannot read " + path.string());
  return {std::istreambuf_iterator<char>(in), {}};
}

void write(const fs::path& path, const std::function<void(std::ostream&)>& fill) {
  std::ofstream out(path, std::ios::binary);
  if (out)
    fill(out);
  out.close();
  if (!out)
    throw std::runtime_error("cannot write " + path.string());
}

namespace {

std::vector<std::string> responses(const std::string& out, std::size_t tokens) {
  std::vector<std::string> got;
  auto at = out.find("\n=>");
  while (at != std::string::npos && got.size() < tokens) {
    const auto start = at + 3, next = out.find("\n=>", start);
    if (next == std::string::npos)
      break;
    got.push_back(out.substr(start, next + 1 - start));
    at = next;
  }
  if (got.size() != tokens)
    throw std::runtime_error(std::format("{} answers for {} tokens", got.size(), tokens));
  return got;
}

std::map<std::string, std::string> readSharedAnswers(const fs::path& captures) {
  std::map<std::string, std::string> answers;
  if (!fs::is_directory(captures / "answers"))
    return answers;
  for (const auto& e : fs::directory_iterator(captures / "answers")) {
    if (e.path().filename().string().starts_with('.'))
      continue;
    std::istringstream in(readFile(e.path() / "tokens"));
    std::vector<std::string> tokens;
    for (std::string t; std::getline(in, t);)
      tokens.push_back(t);
    auto batchAnswers = responses(readFile(e.path() / "ada.out"), tokens.size());
    for (std::size_t t = 0; t < tokens.size(); ++t) {
      auto [at, added] = answers.try_emplace(tokens[t], std::move(batchAnswers[t]));
      if (!added && at->second != batchAnswers[t])
        throw std::runtime_error("conflicting shared answers for " + tokens[t]);
    }
  }
  return answers;
}

} // namespace

std::map<std::string, Seen> load(const fs::path& captures, std::vector<std::string>& names, std::uint64_t& bookTokens,
                                 bool skipIncomplete) {
  if (names.empty())
    for (const auto& e : fs::directory_iterator(captures))
      if (fs::is_regular_file(e.path() / "tokens"))
        names.push_back(e.path().filename());
  std::ranges::sort(names);
  if (names.empty())
    throw std::runtime_error("no captures in " + captures.string());
  const auto sharedAnswers = readSharedAnswers(captures);
  std::map<std::string, Seen> spellings;
  for (const auto& name : names) {
    std::istringstream in(readFile(captures / name / "tokens"));
    std::vector<std::string> tokens;
    for (std::string t; std::getline(in, t);)
      tokens.push_back(t);
    std::vector<std::string> answers;
    try {
      if (fs::is_regular_file(captures / name / "ada.out")) {
        answers = responses(readFile(captures / name / "ada.out"), tokens.size());
      } else {
        for (const auto& t : tokens) {
          const auto at = sharedAnswers.find(t);
          if (at == sharedAnswers.end())
            throw std::runtime_error("no shared answer for " + t);
          answers.push_back(at->second);
        }
      }
    } catch (const std::runtime_error& error) {
      if (!skipIncomplete)
        throw;
      std::cerr << "skipped " << name << ": " << error.what() << '\n';
      continue;
    }
    bookTokens += tokens.size();
    for (std::size_t t = 0; t < tokens.size(); ++t) {
      const auto shared = sharedAnswers.find(tokens[t]);
      if (shared != sharedAnswers.end() && shared->second != answers[t])
        throw std::runtime_error("conflicting saved answers for " + tokens[t] + " in " + name + " and answers/");
      auto& s = spellings[tokens[t]];
      if (s.tokens == 0)
        s.response = answers[t];
      // NOTE: Two answers for one spelling leave no reference to compare with.
      else if (s.response != answers[t])
        throw std::runtime_error("conflicting saved answers for " + tokens[t] + " in " + name);
      ++s.tokens;
    }
  }
  return spellings;
}

int importAnswers(const fs::path& captures, const fs::path& out) {
  std::vector<std::string> names;
  for (const auto& e : fs::directory_iterator(captures))
    if (fs::is_regular_file(e.path() / "ada.out"))
      names.push_back(e.path().filename());
  if (names.empty())
    throw std::runtime_error("no per-book answers in " + captures.string());
  std::uint64_t bookTokens = 0;
  const auto spellings = load(captures, names, bookTokens, true);
  const auto sharedAnswers = readSharedAnswers(captures);
  std::uint64_t added = 0;
  write(out / "tokens", [&](std::ostream& f) {
    for (const auto& word : spellings | std::views::keys)
      if (!sharedAnswers.contains(word)) {
        f << word << '\n';
        ++added;
      }
  });
  write(out / "ada.out", [&](std::ostream& f) {
    f << "\n=>";
    for (const auto& [word, seen] : spellings)
      if (!sharedAnswers.contains(word))
        f << seen.response << "=>";
    f << '\n';
  });
  write(out / "info", [&](std::ostream& f) {
    for (const auto& name : names)
      f << "book: " << name << '\n';
  });
  std::cout << added << " saved answers imported\n";
  return 0;
}

} // namespace lines
