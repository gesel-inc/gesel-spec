# Gesel database file specification (0.1.0)

## Overview

This document describes version 0.1.0 of the Gesel database file formats.
Each Gesel database instance consists of several files, each of which contains different information about the gene set collections. 

## General rules

For validation purposes, it is assumed that all files associated with a single Gesel database instance are located in the same directory.
Note that this does not include the Gesel gene files, which may/should be stored in a separate location depending on the Gesel gene file specification version.

All files associated with a Gesel database instance should have the same prefix.
This allows multiple instances to be stored in the same directory (e.g., for different species), distinguished by their prefix.
In the rest of this document, we will consider a prefix of `PREFIX_` when discussing the names of expected files, though any prefix may be used.
For example, a typical prefix might consist of species' NCBI taxonomy ID plus an underscore, e.g., `9606_collections.tsv.gz` for human.

All files are assumed to use UTF-8 encoding.
The contents of each file should use LF-only newlines (i.e., `\n`).
Each file should also be newline-terminated.
None of the free-text fields in any of the files should contain special characters like `\t` or `\n`.

When storing arrays of integers in a tab-separated file, we use delta-encoding to reduce the number of bytes used by each line.
Given a sorted integer array, we store the first value as the first field of the line.
All subsequent tab-separated fields contain differences from the preceding value in the array.
The original integer array can be recovered as the cumulative sum of the delta-encoded array.
An empty line represents a zero-length array. 

All integers in this document are assumed to fit into a 64-bit unsigned integer.
This includes the cumulative sums used to compute the byte ranges (see next) or to reverse the delta encoding.
The number of lines in any file is also assumed to fit into a 64-bit unsigned integer. 

Each `XXX.tsv.ranges.gz` file stores the number of bytes for each line of the `XXX.tsv` file, excluding the trailing newline.
This can be used in HTTP range requests to extract a single line,
by adding 1 to the number of bytes (for the newline) and computing the cumulative sum to identify the end position of each line.

## Relation to gene indices

Each Gesel database instance should be accompanied by a Gesel gene annotation instance.
Each gene is represented by a "gene index",
which is the 0-based index of the line containing the gene's identities in any of the gene identifier files in the accompanying Gesel gene annotation instance.
All gene indices should be less than the total number of available genes in the gene annotation.
See any version of the Gesel gene file specification (`gene_*.md`) for more details.

## Expected files

### Collection details

`PREFIX_collections.tsv.gz` is a Gzip-compressed tab-separated file where each line corresponds to a gene set collection and contains the following fields:

- `title`: a free-text field containing the title of the collection.
- `description`: a free-text field containing the description of the collection.
- `species`: an integer specifying the NCBI taxonomy ID of the species of the collection.
- `maintainer`: the maintainer of the collection.
- `source`: a free-text field containing the source of the collection.
  This is usually a URL but the exact format is left to the discretion of the database maintainer.
- `number`: the number of gene sets in this collection.

Each collection's identity (i.e., the "collection index") is defined as the 0-based index of the corresponding line in `PREFIX_collections.tsv.gz`.

`PREFIX_collections.tsv` is an uncompressed tab-separated file that has the same number of lines and order of collections as `PREFIX_collections.tsv.gz`.
It contains all fields in `PREFIX_collections.tsv.gz` except for `number`.

`PREFIX_collections.tsv.ranges.gz` is a Gzip-compressed file where each line corresponds to a line (i.e., collection) in `PREFIX_collections.tsv`.
Each line contains the following fields:

- `bytes`: the number of bytes taken up by the corresponding line in `PREFIX_collections.tsv` (excluding the newline).
- `number`: the number of gene sets in this collection.

Applications can either download `PREFIX_collections.tsv.gz` to obtain information about all collections,
or they can download `PREFIX_collections.tsv.ranges.gz` and perform HTTP range requests on `PREFIX_collections.tsv` to obtain information about individual collections.
The former pays a higher up-front cost for easier batch processing.

Developers of Gesel clients may notice that the `start` field is missing from these files. 
This field contains the set index (see below) of the first set in each collection.
To reduce the download size, we do not store `start` in `collections.tsv(.gz)`,
as this can be computed easily on the client from the cumulative sum of `number`.

### Set details

`PREFIX_sets.tsv.gz` is a Gzip-compressed tab-separated file where each line corresponds to a gene set and contains the following fields:

- `name`: a free-text field containing the name of the set.
- `description`: a free-text field containing the description of the set.
- `size`: the number of genes in the set.

Each set's identity (i.e., the "set index") is defined as the 0-based index of the corresponding line in `PREFIX_sets.tsv.gz`.
Sets from the same collection are always stored in consecutive lines, ordered by their position within that collection.

`PREFIX_sets.tsv` is an uncompressed tab-separated file that has the same number of lines and order of sets as `PREFIX_sets.tsv.gz`.
It contains all fields in `PREFIX_sets.tsv.gz` except for `size`.

`PREFIX_sets.tsv.ranges.gz` is a Gzip-compressed file where each line corresponds to a set in `PREFIX_sets.tsv`.
Each line contains two tab-separated fields:

- `bytes`: the number of bytes taken up by the corresponding line in `PREFIX_sets.tsv` (excluding the newline).
- `size`: the number of genes in the set.

Applications can either download `PREFIX_sets.tsv.gz` to obtain information about all sets,
or they can download `PREFIX_sets.tsv.ranges.gz` and perform HTTP range requests on `PREFIX_sets.tsv` to obtain information about individual sets.
The former pays a higher up-front cost for easier batch processing.

Developers of Gesel clients may notice that the `collection` and `number` fields is missing from these files. 
The `collection` field contains the collection index for the collection in which each set belongs,
while the `number` field contains the position of each set in its collection (e.g., 0 for the first set, 1 for the second, and so on).
To reduce the download size, we do not store `collection` and `position` in `PREFIX_sets.tsv.gz`,
as these can be computed easily on the client from the `number` field in `PREFIX_collections.tsv.gz` or `PREFIX_collections.tsv.ranges.gz`.

### Mappings between sets and genes

`PREFIX_set2gene.tsv` is a tab-separated file where each line corresponds to a gene set in `PREFIX_sets.tsv(.gz)`.
Each line contains the delta-encoded gene indices for the corresponding set.
Gene indices for each set should be unique.

`PREFIX_set2gene.tsv.ranges.gz` is a Gzip-compressed file where each line corresponds to a set in `PREFIX_set2gene.tsv`.
Each line contains an integer specifying the number of bytes taken up by the corresponding line in `PREFIX_set2gene.tsv` (excluding the newline).

`PREFIX_gene2set.tsv` is a tab-separated file where the `i`-th line corresponds to a gene index of `i`.
Each line contains the delta-encoded set indices for all sets that contain the corresponding gene.
Set indices for each gene should be unique.

`PREFIX_gene2set.tsv.ranges.gz` is a Gzip-compressed file where each line corresponds to a set in `PREFIX_gene2set.tsv`.
Each line contains an integer specifying the number of bytes taken up by the corresponding line in `PREFIX_gene2set.tsv` (excluding the newline).

`PREFIX_set2gene.tsv.gz` is a Gzip-compressed version of `PREFIX_set2gene.tsv`.
Similarly, `PREFIX_gene2set.tsv.gz` is a Gzip-compressed version of `PREFIX_gene2set.tsv`.

Applications can either download the `*.tsv.gz` files to obtain all relationships up-front,
or they can download `*.ranges.gz` and perform HTTP range requests on the corresponding `*.tsv` to obtain each individual relationship.

### Text search tokens

To search sets based on the names and descriptions, we split the free text into tokens.
The tokenization strategy is very simple - every contiguous stretch of ASCII alphanumeric characters or dashes (`-`) is converted to lower case and treated as a token.
Query strings should be processed in the same manner to generate tokens for matching against `token`.
Handling of `?` or `*` wildcards is at the discretion of the client implementation.

`PREFIX_tokens-names.tsv` is a tab-separated file where each line corresponds to a token.
Each line contains the delta-encoded set indices for all sets that contain the corresponding token in their names.
Set indices for each token should be unique.

`PREFIX_tokens-descriptions.tsv` is a tab-separated file where each line corresponds to a token.
Each line contains the delta-encoded set indices for all sets that contain the corresponding token in their descriptions.
Set indices for each token should be unique.

`PREFIX_tokens-names.tsv.ranges.gz` is a Gzip-compressed file where each line corresponds to a set in `PREFIX_tokens-names.tsv`.
Each line contains:

- `token`: a token string.
  These should be lexicographically sorted, i.e., comparing byte-by-byte using the numerical value of each byte.
- `number`: an integer specifying the number of bytes taken up by the corresponding line in `PREFIX_tokens-names.tsv` (excluding the newline).

The same logic applies to `PREFIX_tokens-descriptions.tsv.ranges.gz` for `PREFIX_tokens-descriptions.tsv`.
This can be used for HTTP range requests to obtain the identities of the sets that match tokens in their names or descriptions.

`PREFIX_tokens-names.tsv.gz` is a Gzip-compressed version of `PREFIX_tokens-names.tsv`.
Similarly, `PREFIX_tokens-descriptions.tsv.gz` is a Gzip-compressed version of `PREFIX_tokens-descriptions.tsv`.

Applications can either download these `*.tsv.gz` files to obtain all relationships up-front,
or they can download `*.ranges.gz` and perform HTTP range requests on the corresponding `*.tsv` to obtain each individual relationship.
