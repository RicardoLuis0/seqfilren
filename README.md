# Sequential file renamer
Usage: `seqfilren [options] ...input files/folders`

If no inputs are specified, will scan current folder

Options:
| SHORT         | LONG                  | DESCRIPTION                                                                                                                                       |
| ------------  | --------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------- |
| -h            | --help                | display help message                                                                                                                              |
| -s            | --same_index          | use the same index for files with different extensions                                                                                            |
| -r            | --recursive           | scan subfolders recursively                                                                                                                       |
| -t            | --test                | don't perform any operations, only display actions that would have been taken                                                                     |
| -c            | --copy                | don't rename/move files, copy them instead                                                                                                        |
| -y            | --yes                 | don't prompt for confirmation                                                                                                                     |
| -p=[prefix]   | --prefix=[prefix]     | specify output file prefix                                                                                                                        |
| -o=[folder]   | --output=[folder]     | specify output folder                                                                                                                             |
| -e=[ext]      | --extension=[ext]     | extension to rename, may be defined multiple times, if not defined, will search all files in folder, leading dot not required, but supported      |
| -m=[rxp]      | --match_regex=[rxp]   | require files to match regular expression                                                                                                         |
|               | --match_regex_ext     | include extension in regex match string                                                                                                           |
|               | --match_regex_path    | include relative path to current folder in regex match string                                                                                     |
| -z            | --zero_index          | start file index at zero instead of one, same as -i=0                                                                                             |
| -i=[int]      | --start_index=[int]   | specify exact starting file index                                                                                                                 |
