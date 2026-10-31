# WORDS fix notes

A quick list of Ada WORDS bugs found while developing whitakerLib.

- [Oracle README](https://github.com/ddd2951/whitakerLib/blob/main/oracle/README.md): comparison method, results and remaining differences.
- [combined_fixes branch](https://github.com/ddd2951/whitakers-words/tree/combined_fixes): the Ada code and regression tests for fixes #1–#19, published as one combined commit.

Numbering follows the oracle README. The first five fix names link to their
upstream pull requests.

| # | Fix | What changes | Regression examples |
| --- | --- | --- | --- |
| 1 | [Syncope state][pr151] | Restore dictionary candidates after syncope. | [amasti perseu][05] |
| 2 | [Enclitic syncope][pr152] | Apply syncope to the base before attaching its enclitic. | [amastique][05] |
| 3 | [Comment reader][pr153] | Ignore stale buffer text when reading comments. | [qualicumque][30] |
| 4 | [Slur and tackon state][pr154] | Restore candidates after failed slur or rejected tackon attempts. | [inminet cotto][05]; [egomet aequicum][30] |
| 5 | [vis syncope][pr155] | Accept vis-syncope only for perfect-system verbs. | [amasti; reseret][05] |
| 6 | Enclitic state | Restore failed-attempt state and freshly look up the enclitic base. | [attene; traque][05] |
| 7 | Unique part | Keep the correct part of speech for unique entries. | [eadem; vis][10] |
| 8 | [Compound-number crash][issue70] | Reset numeral flags between word splits, fixing the #70 reproducer. | [bestiasviginti; unietvicensimae][05] |
| 9 | Repeated PACK/UNIQUES | Prevent fixes-only passes from repeating PACK/UNIQUES readings. | [quoque; quicumque][05] |
| 10 | Repeated adverbs | Derive one adverb per qualifying record, preserving its meaning. | [die; magneque][05] |
| 11 | Unique adverb ending | Skip regular adverb derivation for unique adjectives. | [mi][05] |
| 12 | Full enclitic result | Keep all 40 trick readings plus their outer enclitic marker. | [famamnotaque][05] |
| 13 | Packon boundary | Match the full packon name: cum and cumque are distinct. | [quocumque; quibuscum][05] |
| 14 | Suffix fallback | Use accumulated suffix results when deciding prefix fallback. | [admetus; appius][05] |
| 15 | Qu grouping | Keep direct readings outside Qu addon groups. | [nequiquam; ecquid][05] |
| 16 | Orphan add-ons | Remove addon markers when all their readings are trimmed. | [tuscis; nune][05] |
| 17 | Qu folding | Fold u/v and i/j consistently in Qu routing. | [cvi; cvivs][05] |
| 18 | Nested tackons | Retain accepted inner tackon markers. | [huiuscemodi; huiuscecemodi][05] |
| 19 | Ending lookup | Fold final v/u and j/i before ending lookup. | [senatv; cuj][05] |

The regression links lead to the existing fixtures in the Ada branch.

## Still open: #20, lost full results

`pilula` and `seconda` can become UNKNOWN when fixed parse arrays fill.
An initial repair regressed on `roboraria` and is excluded from the branch.

[05]: https://github.com/ddd2951/whitakers-words/tree/combined_fixes/test/05_multusque/
[10]: https://github.com/ddd2951/whitakers-words/tree/combined_fixes/test/10_aeneid/
[30]: https://github.com/ddd2951/whitakers-words/tree/combined_fixes/test/30_qualdupes/
[pr151]: https://github.com/mk270/whitakers-words/pull/151
[pr152]: https://github.com/mk270/whitakers-words/pull/152
[pr153]: https://github.com/mk270/whitakers-words/pull/153
[pr154]: https://github.com/mk270/whitakers-words/pull/154
[pr155]: https://github.com/mk270/whitakers-words/pull/155
[issue70]: https://github.com/mk270/whitakers-words/issues/70
