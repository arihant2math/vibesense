# Dataset sources

Sources are assigned to a dataset split by manifest:

- `sources.train.json`
- `sources.validation.json`
- `sources.test.json`

A repository must appear in exactly one manifest.

`0` = `human`
`1` = `ai`

Sample-source `path` entries are directory roots. Supported code files are found
recursively in their subdirectories; some are ignored.