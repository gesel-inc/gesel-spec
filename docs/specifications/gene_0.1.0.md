# Gesel gene annotation file specification (0.1.0)

## Overview

This document describes version 0.1.0 of the Gesel gene annotation file formats.
Each Gesel gene annotation instance consists of several files, each of which contains information about different gene identifiers.

## General rules

For validation purposes, it is assumed that all files associated with a single Gesel gene annotation instance are located in the same directory.
However, this should be a separate directory from that used to store the Gesel database files.
This separation allows the use of arbitrary gene identifier types to be used in the file names (see below) without conflicting with the Gesel database files.

All files associated with a Gesel gene annotation instance should have the same prefix.
This allows multiple instances to be stored in the same directory (e.g., for different species), distinguished by their prefix.
In the rest of this document, we will consider a prefix of `PREFIX_` when discussing the names of expected files, though any prefix may be used.
For example, a typical prefix might consist of species' NCBI taxonomy ID plus an underscore, e.g., `9606_ensembl.tsv.gz` for human.

All files are assumed to use UTF-8 encoding.
The contents of each file should use LF-only newlines (i.e., `\n`).
Each file should also be newline-terminated.
None of the free-text fields in any of the files should contain special characters like `\t` or `\n`.

## Expected files

Each gene can be associated with one, zero or multiple identifier of different types (e.g., Ensembl identifiers, symbols). 
For each type of gene identifier, we expect a gene identifier file named `PREFIX_TYPE.tsv.gz`, e.g., `9606_ensembl.tsv.gz`.
This is a Gzip-compressed tab-separated file where each line corresponds to a gene and the tab-separated fields are the identifiers for that gene.
Different genes may have different number of fields, and an empty line indicates that the gene has no identifiers of that type.

Any type of gene identifier can be used, but most Gesel clients will expect one or more of the following files:

- `PREFIX_ensembl.tsv.gz`, where the fields are Ensembl identifiers associated with that gene.
- `PREFIX_entrez.tsv.gz`, where the fields are Entrez identifiers associated with that gene.
- `PREFIX_symbol.tsv.gz`, where the fields are symbols associated with that gene.

All files should have the same number of lines as they represent different identifiers of the same genes.
Each gene's identity (i.e., the "gene index") is defined as the 0-based index of the corresponding line in each file.
The number of lines in each file is the total number of genes in the gene annotation instance.

At least one gene identifier file should be present in a Gesel gene annotation instance.
