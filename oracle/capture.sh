#!/usr/bin/env bash
set -euo pipefail

if (($# < 4)); then
  echo "usage: capture.sh WORDS_BINARY ADA_DATA_DIR OUT_DIR BOOK.words..." >&2
  exit 2
fi
words=$(realpath "$1")
data=$(realpath "$2")
out=$3
shift 3

developer=(
  "PAUSE_IN_SCREEN_OUTPUT N" # NOTE: A pager would eat the next word as RETURN.
  "SHOW_DICTIONARY Y"        # INFO: GEN>/UNI> before each entry number.
  "SHOW_DICTIONARY_LINE Y"   # INFO: The entry numbers.
  "DO_PEARSE_CODES N"
  "HAVE_STATISTICS_FILE N"
  "WRITE_STATISTICS_FILE N"
  "UPDATE_LOCAL_DICTIONARY N"
  "UPDATE_MEANINGS N"
)
modes=(
  "TRIM_OUTPUT Y"
  "HAVE_OUTPUT_FILE N"
  "WRITE_OUTPUT_TO_FILE N"
  "WRITE_UNKNOWNS_TO_FILE N"
)

line() { printf '%-34s%s\n' "$1" "$2"; }

for book in "$@"; do
  name=$(basename "$book" .words)
  dir=$out/$name
  mkdir -p "$out"
  mkdir "$dir"
  work=$(mktemp -d)
  cp "$data"/* "$work"/

  mdv=$(cat "$work/WORD.MDV")
  for setting in "${developer[@]}"; do
    read -r key value <<<"$setting"
    if [[ $(grep -c "^$key " <<<"$mdv") != 1 ]]; then
      echo "WORD.MDV must hold $key exactly once" >&2
      exit 1
    fi
    mdv=$(sed "s/^$key .*/$(line "$key" "$value")/" <<<"$mdv")
  done
  printf '%s\n' "$mdv" >"$work/WORD.MDV"
  {
    if [[ -f $work/WORD.MOD ]]; then
      keys=$(printf '%s\n' "${modes[@]}" | cut -d' ' -f1 | paste -sd'|')
      grep -Ev "^($keys) " "$work/WORD.MOD" || true
    fi
    for setting in "${modes[@]}"; do
      read -r key value <<<"$setting"
      line "$key" "$value"
    done
  } >"$work/WORD.MOD.new"
  mv "$work/WORD.MOD.new" "$work/WORD.MOD"

  tr -d '\r' <"$book" | grep -v '^$' >"$dir/tokens"
  if grep -qv '^[A-Za-z]\+$' "$dir/tokens"; then
    echo "$book: tokens must be ASCII letters only" >&2
    exit 1
  fi
  cp "$work/WORD.MDV" "$work/WORD.MOD" "$dir"/
  {
    echo "words: $words"
    echo "data: $data"
    echo "book: $(realpath "$book")"
    echo "tokens: $(wc -l <"$dir/tokens")"
    echo "date: $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "words sha256: $(sha256sum "$words" | cut -d' ' -f1)"
    if [[ -f $(dirname "$words")/SOURCE ]]; then
      echo "source:"
      sed 's/^/  /' "$(dirname "$words")/SOURCE"
    fi
  } >"$dir/info"
  (cd "$work" && sha256sum -- *) >"$dir/data.sha256"

    # INFO: Two blank lines end WORDS.
  start=$SECONDS
  { cat "$dir/tokens"; printf '\n\n'; } |
    (cd "$work" && WHITAKERS_WORDS_DATADIR=. "$words") >"$dir/ada.out" 2>"$dir/ada.err"
  rm -rf "$work"
  echo "$name: $(wc -l <"$dir/tokens") tokens in $((SECONDS - start))s -> $dir"
done
