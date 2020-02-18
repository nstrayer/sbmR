## Does not work as of 2-18-2020

#Downloading the data from ENCODExplorer.

#Have to use unreleased package version from github to get newest database

# devtools::install_github("CharlesJB/ENCODExplorer")

library(ENCODExplorer)
library(tidyverse)
library(here)

bed_names = c("chrom",
              "chromStart",
              "chromEnd",
              "name",
              "score",
              "strand",
              "thickStart",
              "thickEnd",
              "itemRGB",
              "blockCount")

data(encode_df, package = "ENCODExplorer")

query_results <- queryEncode(df = encode_df, assay = "eCLIP", file_format = "bed", fixed = F)

sub_query <- query_results %>%
  filter(biological_replicates == "1; 2" &
           biosample_name == "HepG2")


dir.create("eCLIP_datasets HepG2")

downloadEncode(sub_query, encode_df, dir = here("eCLIP_datasets HepG2"))



read_encode_beds <- function(bed_file) {
  test <- read_tsv(here("eCLIP_datasets HepG2", bed_file), col_names = bed_names) %>%
    select(c("chrom",
             "chromStart",
             "chromEnd",
             "name",
             "score",
             "strand"))
}

RBP_eCLIP_peaks <- map_df(list.files(here("eCLIP_datasets HepG2")), read_encode_beds)

# write_tsv(RBP_eCLIP_peaks, here("RBP_eCLIP_peaks_HepG2.bed"), col_names = FALSE)
#
#
# #USE THIS BEDTOOLS CALL:
#
# #bedtools intersect -wb -f 0.50 -a RBP_eCLIP_peaks.bed -b hg19_gtf.bed > RBP_eCLIP_named.bed
#
# #CHANGE: used UCSC Data integrator with NCBIRefseq genes, select name2 column
#
#
#
# RBP_eCLIP_final_HepG2 <- RBP_eCLIP_named_HepG2 %>%
#   select("RBP" = X4, "Target" = X5) %>%
#   unique() %>%
#   filter(!is.na(Target))
#
# saveRDS(RBP_eCLIP_final_HepG2, file = "RBP_eCLIP_HepG2_052119.rda")
#
#
# #Integrating m6A data from Dominissini 2012
#
# m6A_annotation <- read_tsv("Dominissini_m6A_genes_hg19", col_names = FALSE, skip = 2) %>%
#   filter(!is.na(X4)) %>%
#   select("Target" = X4) %>%
#   unique() %>%
#   mutate(m6A_Modified = 1)
#
# #Join with eCLIP data for HepG2
#
#
# RBP_eCLIP_HepG2_m6A <- RBP_eCLIP_final_HepG2 %>%
#   select(Target) %>%
#   unique() %>%
#   full_join(m6A_annotation, by = "Target") %>%
#   mutate(m6A_Modified = replace_na(m6A_Modified, 0))
#
#
# saveRDS(RBP_eCLIP_HepG2_m6A, file = "RBP_eCLIP_HepG2_m6A_052119.rda")
#
#
#
#




# usethis::use_data("rna_binding_sites")
