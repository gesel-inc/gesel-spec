# gesel file specification v0.1.0

## Overview

The [Gesel database](https://doi.org/10.21105/joss.05777) uses client-side HTTP range requests to extract gene sets and other details.
This document describes the file formats used by Gesel; once created, the files can be hosted on a static file server without requiring any more logic.

Each species contains a separate copy of the files described in this section.
Files from a particular species will be prefixed with that species' NCBI taxonomy ID, e.g., `9606_ensembl.tsv.gz`.
For brevity, we will omit the prefix in the rest of this document.

The contents of each file should use LF-only newlines (i.e., `\n`).
Each file should also be newline-terminated.
None of the free-text fields in any of the files should contain special characters like `\t` or `\n`.

Each `XXX.tsv.ranges.gz` file stores the number of bytes for each line of the `XXX.tsv` file, excluding the trailing newline.
This can be used in HTTP range requests to extract a single line,
by adding 1 to the number of bytes (for the newline) and computing the cumulative sum to identify the end position of each line.

We use delta-encoding to reduce the number of bytes used by each line of tab-separated integers.
Given a sorted integer array, we store the first value as the first field of the line.
All subsequent tab-separated fields contain differences from the preceding value in the array.
The original integer array can be recovered as the cumulative sum of the delta-encoded array.

All integers in this document are assumed to fit into a 64-bit unsigned integer.
This includes the cumulative sums used to compute the byte ranges or to reverse the delta encoding.
All strings are assumed to use UTF-8 encoding.

## Gene mappings

Each gene can be associated with one, zero or multiple names of different types (e.g., Ensembl identifiers, symbols). 
For each type of gene name, we expect a file named `<TYPE>.tsv.gz`.
This is a Gzip-compressed tab-separated file where each line corresponds to a gene and the tab-separated fields are the names for that gene.
Different genes may have different number of fields, and an empty line indicates that the equivalence class contains no names of that type.

Any type of gene name can be used, but most Gesel clients will expect one or more of the following files:

- `ensembl.tsv.gz`, where the fields are Ensembl identifiers associated with that gene.
- `entrez.tsv.gz`, where the fields are Entrez identifiers associated with that gene.
- `symbol.tsv.gz`, where the fields are symbols associated with that gene.

All files have the same number of lines as they represent different names of the same genes.
Each gene's identity (i.e., the "gene index") is defined as the 0-based index of the corresponding line in each file.

## Collection details

`collections.tsv.gz` is a Gzip-compressed tab-separated file where each line corresponds to a gene set collection and contains the following fields:

- `title`: a free-text field containing the title of the collection.
- `description`: a free-text field containing the description of the collection.
- `species`: an integer specifying the NCBI taxonomy ID of the species of the collection.
  (This field is not used and is listed here only for back-compatibility.)
- `maintainer`: the maintainer of the collection.
- `source`: a free-text field containing the source of the collection.
  This is usually a URL but the exact format is left to the discretion of the database maintainer.
- `number`: the number of gene sets in this collection.

Each collection's identity (i.e., the "collection index") is defined as the 0-based index of the corresponding line in `collections.tsv.gz`.

`collections.tsv` is an uncompressed tab-separated file that has the same number of lines and order of collections as `collections.tsv.gz`.
It contains all fields in `collections.tsv.gz` except for `number`.

`collections.tsv.ranges.gz` is a Gzip-compressed file where each line corresponds to a collection in `collections.tsv`.
Each line contains the following fields:

- `bytes`: the number of bytes taken up by the corresponding line in `collections.tsv` (excluding the newline).
- `number`: the number of gene sets in this collection.

Applications can either download `collections.tsv.gz` to obtain information about all collections,
or they can download `collections.tsv.ranges.gz` and perform HTTP range requests on `collections.tsv` to obtain information about individual collections.
The former pays a higher up-front cost for easier batch processing.

Developers of Gesel clients may notice that the `start` field is missing from these files. 
This field contains the set index (see below) of the first set in each collection.
To reduce the download size, we do not store `start` in `collections.tsv(.gz)`,
as this can be computed easily on the client from the cumulative sum of `number`.

## Set details

`sets.tsv.gz` is a Gzip-compressed tab-separated file where each line corresponds to a gene set and contains the following fields:

- `name`: a free-text field containing the name of the set.
- `description`: a free-text field containing the description of the set.
- `size`: the number of genes in the set.

Each set's identity (i.e., the "set index") is defined as the 0-based index of the corresponding line in `sets.tsv.gz`.
Sets from the same collection are always stored in consecutive lines, ordered by their position within that collection.

`sets.tsv` is an uncompressed tab-separated file that has the same number of lines and order of sets as `sets.tsv.gz`.
It contains all fields in `sets.tsv.gz` except for `size`.

`sets.tsv.ranges.gz` is a Gzip-compressed file where each line corresponds to a set in `sets.tsv`.
Each line contains two tab-separated fields:

- `bytes`: the number of bytes taken up by the corresponding line in `sets.tsv` (excluding the newline).
- `size`: the number of genes in the set.

Applications can either download `sets.tsv.gz` to obtain information about all sets,
or they can download `sets.tsv.ranges.gz` and perform HTTP range requests on `sets.tsv` to obtain information about individual sets.
The former pays a higher up-front cost for easier batch processing.

Developers of Gesel clients may notice that the `collection` and `number` fields is missing from these files. 
The `collection` field contains the collection index for the collection in which each set belongs,
while the `number` field contains the position of each set in its collection (e.g., 0 for the first set, 1 for the second, and so on).
To reduce the download size, we do not store `collection` and `position` in `sets.tsv.gz`,
as these can be computed easily on the client from the `number` field in `collections.tsv.gz` or `collections.tsv.ranges.gz`.

## Mappings between sets and genes

`set2gene.tsv` is a tab-separated file where each line corresponds to a gene set in `sets.tsv(.gz)`.
Each line contains the delta-encoded gene indices for the corresponding set.
Gene indices for each set should be unique.

`set2gene.tsv.ranges.gz` is a Gzip-compressed file where each line corresponds to a set in `set2gene.tsv`.
Each line contains an integer specifying the number of bytes taken up by the corresponding line in `set2gene.tsv` (excluding the newline).

`gene2set.tsv` is a tab-separated file where each line corresponds to a gene in any of the gene mapping files (e.g., `symbol.tsv.gz`, `ensembl.tsv.gz`).
Each line contains the delta-encoded set indices for all sets that contain the corresponding gene.
Set indices for each gene should be unique.

`gene2set.tsv.ranges.gz` is a Gzip-compressed file where each line corresponds to a set in `gene2set.tsv`.
Each line contains an integer specifying the number of bytes taken up by the corresponding line in `gene2set.tsv` (excluding the newline).

`set2gene.tsv.gz` is a Gzip-compressed version of `set2gene.tsv`.
Similarly, `gene2set.tsv.gz` is a Gzip-compressed version of `gene2set.tsv`.
Applications can either download these `*.tsv.gz` files to obtain all relationships up-front,
or they can download `*.ranges.gz` and perform HTTP range requests on the corresponding `*.tsv` to obtain each individual relationship.

## Text search tokens

To search sets based on the names and descriptions, we split the free text into tokens.
The tokenization strategy is very simple - every contiguous stretch of ASCII alphanumeric characters or dashes (`-`) is treated as a token.
Query strings should be processed in the same manner to generate tokens for matching against `token`.
Handling of `?` or `*` wildcards is at the discretion of the client implementation.

`tokens-names.tsv` is a tab-separated file where each line corresponds to a token.
Each line contains the delta-encoded set indices for all sets that contain the corresponding token in their names.
Set indices for each token should be unique.

`tokens-descriptions.tsv` is a tab-separated file where each line corresponds to a token.
Each line contains the delta-encoded set indices for all sets that contain the corresponding token in their descriptions.
Set indices for each token should be unique.

`tokens-names.tsv.ranges.gz` is a Gzip-compressed file where each line corresponds to a set in `tokens-names.tsv`.
Each line contains:

- `token`: a token string.
  These should be lexicographically sorted, i.e., comparing byte-by-byte using the numerical value of each byte.
- `number`: an integer specifying the number of bytes taken up by the corresponding line in `tokens-names.tsv` (excluding the newline).

The same logic applies to `tokens-descriptions.tsv.ranges.gz` for `tokens-descriptions.tsv`.
This can be used for HTTP range requests to obtain the identities of the sets that match tokens in their names or descriptions.

`tokens-names.tsv.gz` is a Gzip-compressed version of `tokens-names.tsv`.
Similarly, `tokens-descriptions.tsv.gz` is a Gzip-compressed version of `tokens-descriptions.tsv`.
Applications can either download these `*.tsv.gz` files to obtain all relationships up-front,
or they can download `*.ranges.gz` and perform HTTP range requests on the corresponding `*.tsv` to obtain each individual relationship.
