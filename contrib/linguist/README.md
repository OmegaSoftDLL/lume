# Linguist submission preparation

This directory stages the files for a future GitHub Linguist contribution. Lume is not yet in Linguist's catalogue, and the .src extension is intentionally excluded because it is generic and already used by other languages.

## Draft languages.yml entry

    Lume:
      type: programming
      color: "#6A4C93"
      extensions:
      - .lume
      tm_scope: source.lume
      ace_mode: text

The proposed scope matches the TextMate grammar at ../textmate/syntaxes/Lume.tmLanguage.json. The extension check against [Linguist's current languages.yml](https://github.com/github-linguist/linguist/blob/main/lib/linguist/languages.yml) found no .lume entry. Recheck the upstream file when opening the submission.

## Grammar source

The grammar is maintained in this repository at contrib/textmate/syntaxes/Lume.tmLanguage.json and uses the repository's dual MIT OR Apache-2.0 license. The public Lume repository serves as the grammar source. The Linguist grammar integration workflow runs script/add-grammar in a temporary Linguist checkout with Docker, without changing or publishing to github/linguist.

## Samples

samples/Lume/ contains unchanged, non-trivial .lume examples copied from the project. Keep each sample synchronized with its source in examples/ until it is copied into Linguist's own samples/Lume/ during submission.
