# flat_citation
A Drupal module that generates a citation based on available CMDI metadata in Solr, and displays it in a block.

Requires the following fields to be indexed in Solr, as non-tokenized fields:
* ancestors_ms
* PID
* dc.title
* dc.identifier
* cmd.Contributor
* cmd.CreationDate
* cmd.url
* RELS_EXT_hasModel_uri_ms
* RELS_EXT_isConstituentOf_uri_ms

The module assumes that nodes above the individual collection level do not have CMDI metadata