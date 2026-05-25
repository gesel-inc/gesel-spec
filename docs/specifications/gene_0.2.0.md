# Gesel gene annotation file specification (0.2.0)

## Overview

This document describes version 0.2.0 of the Gesel gene annotation file formats.
Each Gesel gene annotation instance consists of several files, each of which contains information about different gene identifiers.

## General rules

For validation purposes, it is assumed that all files associated with a single Gesel gene annotation instance are located in the same directory.
This may also be the same directory as the Gesel database files;
no conflict will occur as all files described here have a `gene-` component that is not present in the database files.
I

All files associated with a Gesel gene annotation instance should have the same prefix.
This allows multiple instances to be stored in the same directory (e.g., for different species), distinguished by their prefix.
In the rest of this document, we will consider a prefix of `PREFIX_` when discussing the names of expected files, though any prefix may be used.
For example, a typical prefix might consist of species' NCBI taxonomy ID plus an underscore, e.g., `9606_gene-type-ensembl.tsv.gz` for human.

All files are assumed to use UTF-8 encoding.
The contents of each file should use LF-only newlines (i.e., `\n`).
Each file should also be newline-terminated.
None of the free-text fields in any of the files should contain special characters like `\t` or `\n`.

## Expected files

### Version

`PREFIX_gene-version.tsv` should contain the `0.2.0` string, followed by a newline.

### Manifest

`PREFIX_gene-types.tsv` should contain the types of the gene identifiers.
Each line should contain one type, which should be a non-empty string that consists of alphanumeric characters only (i.e., `A-Za-z0-9`).
At least one line should be present.

### Gene identifiers

Each gene can be associated with one, zero or multiple names of different types (e.g., Ensembl identifiers, symbols). 
For each type of gene name, we expect a gene identifier file named `PREFIX_gene-type-TYPE.tsv.gz`, e.g., `9606_gene-type-ensembl.tsv.gz`.
This is a Gzip-compressed tab-separated file where each line corresponds to a gene and the tab-separated fields are the names for that gene.
Different genes may have different number of fields, and an empty line indicates that the equivalence class contains no names of that type.

Any type of gene identifier can be used.
One gene identifier file should be present for each type listed in `PREFIX_gene-types.tsv`.
All files should have the same number of lines as they represent different names of the same genes.
Each gene's identity (i.e., the "gene index") is defined as the 0-based index of the corresponding line in each file.
The number of lines in each file is the total number of genes in the gene annotation instance.

At least one gene identifier file should be present in a Gesel gene annotation instance.
