<?php

/**
 * @author  Paul Trilsbeek, Ibrahim Abdullah
 * @package Flat Citation
 * Generates a citation from available CMDI metadata in Solr
 */
class CitationGenerator
{
    /**
     * @var Apache_Solr_Service
     */
    private $solr;

    /**
     * Connect to apache solr
     *
     * @return Apache_Solr_Service
     */
    public function connect()
    {
        if (null !== $this->solr) {
            return $this->solr;
        }

        $path_parts = parse_url(variable_get('islandora_solr_url', 'localhost:8080/solr'));

        $this->solr = new Apache_Solr_Service($path_parts['host'], $path_parts['port'], $path_parts['path'] . '/');
        $this->solr->setCreateDocuments(0);

        return $this->solr;
    }

    /**
     * Common solr query performer
     *
     * @param string $query
     * @param array $params
     *
     * @return array
     */
    public function query($query, $params = [])
    {
        $this->connect();

        try {
            $results = $this->solr->search($query, 0, 1000, $params);
        } catch (Exception $e) {

            drupal_set_message(check_plain(t('Error searching Solr index')) . ' ' . $e->getMessage(), 'error');
            return false;
        }

        return json_decode($results->getRawResponse(), true);
    }

    /**
     * Get ancestors for given pid and return them including pid itself
     *
     * @param string $pid
     *
     * @return array
     */
    public function ancestors($pid)
    {
        $field = 'ancestors_ms';
        $solrParams = [];
        // Restrict results based on specified namespaces.
        $namespace_list = trim(variable_get('islandora_solr_namespace_restriction', ''));
        if ($namespace_list) {
            $namespaces = preg_split('/[,|\s]/', $namespace_list);
            $namespace_array = array();
            foreach (array_filter($namespaces) as $namespace) {
                $namespace_array[] = "PID:$namespace\:*";
            }
            $solrParams['fq'][] = implode(' OR ', $namespace_array);
        }

        $solrParams['fl'][] = $field;

        $data  = $this->query('PID:"' . $pid . '"', $solrParams);

        if (!array_key_exists($field, $data['response']['docs'][0])) {
            return;
        }

        $result = $data['response']['docs'][0][$field];

        if (!empty($result)) {
            //add pid itself as we also want info about it

            array_unshift($result, $pid);
        }

        return $result;
    }

    /**
     * Get info on each ancestor for building the citation
     *
     * @param string $ancestors
     *
     * @return array
     */
    public function ancestorInfo($ancestors)
    {
        $data     = $this->query($this->ancestorsQuery($ancestors), $this->ancestorsParams());

        if ($data['response']['numFound'] === 0) {
            return false;
        }

        if (!isset($data['response']['docs'])) {
            return false;
        }

        return $data['response']['docs'];
    }

    /**
     * Build query for getting details for each ancestor
     *
     * @param string $parent
     *
     * @return string
     */
    public function ancestorsQuery($ancestors)
    {

        $query_values = [];

        if (!empty($ancestors)) {

            foreach ($ancestors as $ancestor) {


                $query_values[] = format_string('PID:("info:fedora/!ancestor_pid" OR "!ancestor_pid")', [
                    '!ancestor_pid' => $ancestor,
                ]);
            }
        }

        return implode(" OR ", $query_values);
    }

    /**
     * Query params (fields we want returned from Solr in order to build the citation)
     *
     * @param string $parent
     *
     * @return array
     */
    public function ancestorsParams()
    {

        $solrParams['fl'][] = 'PID,
            dc.title,
            cmd.Contributor,
            cmd.CreationDate,
            cmd.url,
            RELS_EXT_hasModel_uri_ms,
            RELS_EXT_isConstituentOf_uri_ms,
            dc.identifier,
            CMD_Components_lat-corpus_Citation_Authors_ms,
            CMD_Components_lat-corpus_Citation_Date_ms,
            CMD_Components_lat-corpus_Citation_Title_ms,
            CMD_Components_lat-corpus_Citation_Comment_ms,
            CMD_Components_lat-session_Citation_Authors_ms,
            CMD_Components_lat-session_Citation_Date_ms,
            CMD_Components_lat-session_Citation_Title_ms,
            CMD_Components_lat-session_Citation_Comment_ms,
            CMD_Components_MPI_Collection_Citation_Authors_ms,
            CMD_Components_MPI_Collection_Citation_Date_ms,
            CMD_Components_MPI_Collection_Citation_Title_ms,
            CMD_Components_MPI_Collection_Citation_Comment_ms,
            CMD_Components_MPI_Bundle_Citation_Authors_ms,
            CMD_Components_MPI_Bundle_Citation_Date_ms,
            CMD_Components_MPI_Bundle_Citation_Title_ms,
            CMD_Components_MPI_Bundle_Citation_Comment_ms,
            CMD_Components_lat-SL-session_Citation_Authors_ms,
            CMD_Components_lat-SL-session_Citation_Date_ms,
            CMD_Components_lat-SL-session_Citation_Title_ms,
            CMD_Components_lat-SL-session_Citation_Comment_ms,
            CMD_Components_lat-corpus_RelatedCitation_Authors_ms,
            CMD_Components_lat-corpus_RelatedCitation_Date_ms,
            CMD_Components_lat-corpus_RelatedCitation_Title_ms,
            CMD_Components_lat-corpus_RelatedCitation_Publication_ms,
            CMD_Components_lat-corpus_RelatedCitation_Publisher_ms,
            CMD_Components_lat-corpus_RelatedCitation_PersistentIdentifier_ms,
            CMD_Components_lat-corpus_RelatedCitation_Comment_ms,
            CMD_Components_lat-session_RelatedCitation_Authors_ms,
            CMD_Components_lat-session_RelatedCitation_Date_ms,
            CMD_Components_lat-session_RelatedCitation_Title_ms,
            CMD_Components_lat-session_RelatedCitation_Publication_ms,
            CMD_Components_lat-session_RelatedCitation_Publisher_ms,
            CMD_Components_lat-session_RelatedCitation_PersistentIdentifier_ms,
            CMD_Components_lat-session_RelatedCitation_Comment_ms,
            CMD_Components_MPI_Collection_RelatedCitation_Authors_ms,
            CMD_Components_MPI_Collection_RelatedCitation_Date_ms,
            CMD_Components_MPI_Collection_RelatedCitation_Title_ms,
            CMD_Components_MPI_Collection_RelatedCitation_Publication_ms,
            CMD_Components_MPI_Collection_RelatedCitation_Publisher_ms,
            CMD_Components_MPI_Collection_RelatedCitation_PersistentIdentifier_ms,
            CMD_Components_MPI_Collection_RelatedCitation_Comment_ms,
            CMD_Components_MPI_Bundle_RelatedCitation_Authors_ms,
            CMD_Components_MPI_Bundle_RelatedCitation_Date_ms,
            CMD_Components_MPI_Bundle_RelatedCitation_Title_ms,
            CMD_Components_MPI_Bundle_RelatedCitation_Publication_ms,
            CMD_Components_MPI_Bundle_RelatedCitation_Publisher_ms,
            CMD_Components_MPI_Bundle_RelatedCitation_PersistentIdentifier_ms,
            CMD_Components_MPI_Bundle_RelatedCitation_Comment_ms,
            CMD_Components_lat-SL-session_RelatedCitation_Authors_ms,
            CMD_Components_lat-SL-session_RelatedCitation_Date_ms,
            CMD_Components_lat-SL-session_RelatedCitation_Title_ms,
            CMD_Components_lat-SL-session_RelatedCitation_Publication_ms,
            CMD_Components_lat-SL-session_RelatedCitation_Publisher_ms,
            CMD_Components_lat-SL-session_RelatedCitation_PersistentIdentifier_ms,
            CMD_Components_lat-SL-session_RelatedCitation_Comment_ms';

        return $solrParams;
    }

    /**
     * function to turn cmd.CreationDate dates into date (year) range
     *
     * @param array $dates
     *
     * @return string
     */
    public function dateRange($dates)
    {
        $years = [];
        foreach ($dates as $date) {
            // year only
            if (strpos($date, "Un") !== false) {
                continue;
            }
            $year = substr($date, 0, 4);
            $years[] = $year;
        }
        sort($years);
        $lastYear = end($years);
        $firstYear = reset($years);

        if ($firstYear == $lastYear) {
            return $firstYear;
        } else {
            return $firstYear . " - " . $lastYear;
        }
    }

    /**
     * function to create a formatted list of authors
     *
     * @param array $authors
     *
     * @return string
     */
    public function authorsList($authors)
    {

        if (sizeof($authors) <= 1) {
            return implode("", $authors);
        }

        if (sizeof($authors) == 2) {
            return implode(' and ', $authors);
        }

        if (sizeof($authors) > 2) {
            $last_author = end($authors);
            $all_but_last_authors = array_slice($authors, 0, -1);
            return implode(', ', $all_but_last_authors) . ', and ' . $last_author;
        }
    }

    /**
     * function to turn persistent identifier into actionable link, if possible
     * supporting doi: and hdl: prefixes
     * 
     * @param string $pid
     * 
     * @return string
     */
    public function pidToLink($pid)
    {
        $url = "";
        if (stripos($pid, "http://") !== false OR stripos($pid, "https://") !== false) {
            $url = $pid;
        }
        elseif (stripos($pid, "doi:") == 0) {
            $url = str_ireplace("doi:", "https://doi.org/", $pid);
        }
        elseif (stripos($pid, "hdl:") == 0) {
            $url = str_ireplace("hdl:", "https://hdl.handle.net/", $pid);
        }
        if (!empty($url)) {
            return "<a href='" . $url . "' alt='Persistent identifier for related publication'>" . $pid . "</a>";
        }
        return $pid;
    }

    /**
     * Generating citation
     *
     * @param string $pid
     *
     * @return array|false
     */
    public function generate($pid)
    {

        $today = date("Y-m-d");
        $authors = "[author(s)]"; // default value for authors. In case author info cannot be found, we still show the remainder of the citation.
        $overruling_authors = "";
        $date = "";
        $overruling_date = "";
        $title = "";
        $bundle_title = "";
        $collection_title = "";
        $url = "";
        $type = "";
        $models = "";

        $result = "<h2>Citation</h2>";

        $message = "This citation was extracted automatically from the available metadata and may contain inaccuracies. In case of multiple authors, the ordering is arbitrary.
         Please contact the archive staff in case you need help on how to cite this resource.";

        $no_citation_message = "A citation could not be generated automatically with the available metadata. Please contact the archive staff for help on how to cite this resource.";

        $current_path = current_path();

        // hide citation if not on object view page (path has additional segment)
        $hide = preg_match('/.*islandora\/object\/.+\/.+/', $current_path);

        if ($hide) {
            return "";
        }

        $ancestors = $this->ancestors($pid);

        if (!empty($ancestors)) {

            $info = $this->ancestorInfo($ancestors);

            if (empty($info)) {
                // root collection, no citation needed
                return;
            }

            if (!empty($info)) {

                $key = array_search($ancestors[0], array_column($info, 'PID'));

                // See whether the current object is a Collection, Bundle or File, and whether it has a CMDI content model

                $type = "file"; // default 

                if (array_key_exists('RELS_EXT_hasModel_uri_ms', $info[$key])) {

                    $models = $info[$key]['RELS_EXT_hasModel_uri_ms'];

                    if (in_array("info:fedora/islandora:collectionCModel", $models) and !in_array("info:fedora/islandora:sp_cmdiCModel", $models)) {
                        // object is a higher level collection that does not have CMDI metadata, no citation needed
                        return;
                    }

                    if (in_array("info:fedora/islandora:collectionCModel", $models)) {
                        $type = "collection";
                    }

                    if (in_array("info:fedora/islandora:compoundCModel", $models) and !in_array("info:fedora/islandora:collectionCModel", $models)) {
                        $type = "bundle";
                    }
                }

                // title of the current pid, from overruling citation or otherwise dc.title

                if (array_key_exists('CMD_Components_lat-corpus_Citation_Title_ms', $info[$key])) {

                    $title = $info[$key]['CMD_Components_lat-corpus_Citation_Title_ms'][0];
                } elseif (array_key_exists('CMD_Components_lat-session_Citation_Title_ms', $info[$key])) {

                    $title = $info[$key]['CMD_Components_lat-session_Citation_Title_ms'][0];
                } elseif (array_key_exists('CMD_Components_MPI_Collection_Citation_Title_ms', $info[$key])) {

                    $title = $info[$key]['CMD_Components_MPI_Collection_Citation_Title_ms'][0];
                } elseif (array_key_exists('CMD_Components_MPI_Bundle_Citation_Title_ms', $info[$key])) {

                    $title = $info[$key]['CMD_Components_MPI_Bundle_Citation_Title_ms'][0];
                } elseif (array_key_exists('CMD_Components_lat-SL-session_Citation_Title_ms', $info[$key])) {

                    $title = $info[$key]['CMD_Components_lat-SL-session_Citation_Title_ms'][0];
                } elseif (array_key_exists('dc.title', $info[$key])) {

                    $title = $info[$key]['dc.title'][0];
                }

                // url of the current pid

                if (array_key_exists('cmd.url', $info[$key])) {

                    $url = $info[$key]['cmd.url'][0];
                }


                // get first citation comment bottom up (in case of file, we'll check later at the bundle level)

                foreach ($ancestors as $ancestor) {

                    $key = array_search($ancestor, array_column($info, 'PID'));

                    if (array_key_exists('CMD_Components_lat-corpus_Citation_Comment_ms', $info[$key])) {

                        $comment = $info[$key]['CMD_Components_lat-corpus_Citation_Comment_ms'][0];
                        break;
                    } elseif (array_key_exists('CMD_Components_MPI_Collection_Citation_Comment_ms', $info[$key])) {

                        $comment = $info[$key]['CMD_Components_MPI_Collection_Citation_Comment_ms'][0];
                        break;
                    } elseif (array_key_exists('CMD_Components_lat-SL-corpus_Citation_Comment_ms', $info[$key])) {

                        $comment = $info[$key]['CMD_Components_lat-SL-corpus_Citation_Comment_ms'][0];
                        break;
                    }
                }

                // get related citation bottom up (in case of file, we'll check later at the bundle level)

                foreach ($ancestors as $ancestor) {

                    $key = array_search($ancestor, array_column($info, 'PID'));

                    // authors
                    if (array_key_exists('CMD_Components_lat-corpus_RelatedCitation_Authors_ms', $info[$key])) {

                        $related_citation_authors = $info[$key]['CMD_Components_lat-corpus_RelatedCitation_Authors_ms'][0];
                    } elseif (array_key_exists('CMD_Components_MPI_Collection_RelatedCitation_Authors_ms', $info[$key])) {

                        $related_citation_authors = $info[$key]['CMD_Components_MPI_Collection_RelatedCitation_Authors_ms'][0];
                    } elseif (array_key_exists('CMD_Components_lat-session_RelatedCitation_Authors_ms', $info[$key])) {

                        $related_citation_authors = $info[$key]['CMD_Components_lat-session_RelatedCitation_Authors_ms'][0];
                    } elseif (array_key_exists('CMD_Components_MPI_Bundle_RelatedCitation_Authors_ms', $info[$key])) {

                        $related_citation_authors = $info[$key]['CMD_Components_MPI_Bundle_RelatedCitation_Authors_ms'][0];
                    } elseif (array_key_exists('CMD_Components_lat-SL-session_RelatedCitation_Authors_ms', $info[$key])) {

                        $related_citation_authors = $info[$key]['CMD_Components_lat-SL-session_RelatedCitation_Authors_ms'][0];
                    }

                    // date
                    if (array_key_exists('CMD_Components_lat-corpus_RelatedCitation_Date_ms', $info[$key])) {

                        $related_citation_date = $info[$key]['CMD_Components_lat-corpus_RelatedCitation_Date_ms'][0];
                    } elseif (array_key_exists('CMD_Components_MPI_Collection_RelatedCitation_Date_ms', $info[$key])) {

                        $related_citation_date = $info[$key]['CMD_Components_MPI_Collection_RelatedCitation_Date_ms'][0];
                    } elseif (array_key_exists('CMD_Components_lat-session_RelatedCitation_Date_ms', $info[$key])) {

                        $related_citation_date = $info[$key]['CMD_Components_lat-session_RelatedCitation_Date_ms'][0];
                    } elseif (array_key_exists('CMD_Components_MPI_Bundle_RelatedCitation_Date_ms', $info[$key])) {

                        $related_citation_date = $info[$key]['CMD_Components_MPI_Bundle_RelatedCitation_Date_ms'][0];
                    } elseif (array_key_exists('CMD_Components_lat-SL-session_RelatedCitation_Date_ms', $info[$key])) {

                        $related_citation_date = $info[$key]['CMD_Components_lat-SL-session_RelatedCitation_Date_ms'][0];
                    }

                    // title
                    if (array_key_exists('CMD_Components_lat-corpus_RelatedCitation_Title_ms', $info[$key])) {

                        $related_citation_title = $info[$key]['CMD_Components_lat-corpus_RelatedCitation_Title_ms'][0];
                    } elseif (array_key_exists('CMD_Components_MPI_Collection_RelatedCitation_Title_ms', $info[$key])) {

                        $related_citation_title = $info[$key]['CMD_Components_MPI_Collection_RelatedCitation_Title_ms'][0];
                    } elseif (array_key_exists('CMD_Components_lat-session_RelatedCitation_Title_ms', $info[$key])) {
                    
                        $related_citation_title = $info[$key]['CMD_Components_lat-session_RelatedCitation_Title_ms'][0];
                    } elseif (array_key_exists('CMD_Components_MPI_Bundle_RelatedCitation_Title_ms', $info[$key])) {
                        
                        $related_citation_title = $info[$key]['CMD_Components_MPI_Bundle_RelatedCitation_Title_ms'][0];
                    } elseif (array_key_exists('CMD_Components_lat-SL-session_RelatedCitation_Title_ms', $info[$key])) {
                        
                        $related_citation_title = $info[$key]['CMD_Components_lat-SL-session_RelatedCitation_Title_ms'][0];
                    }

                    // publication

                    if (array_key_exists('CMD_Components_lat-corpus_RelatedCitation_Publication_ms', $info[$key])) {

                        $related_citation_publication = $info[$key]['CMD_Components_lat-corpus_RelatedCitation_Publication_ms'][0];
                    } elseif (array_key_exists('CMD_Components_MPI_Collection_RelatedCitation_Publication_ms', $info[$key])) {

                        $related_citation_publication = $info[$key]['CMD_Components_MPI_Collection_RelatedCitation_Publication_ms'][0];
                    } elseif (array_key_exists('CMD_Components_lat-session_RelatedCitation_Publication_ms', $info[$key])) {
                        
                        $related_citation_publication = $info[$key]['CMD_Components_lat-session_RelatedCitation_Publication_ms'][0];
                    } elseif (array_key_exists('CMD_Components_MPI_Bundle_RelatedCitation_Publication_ms', $info[$key])) {
                        
                        $related_citation_publication = $info[$key]['CMD_Components_MPI_Bundle_RelatedCitation_Publication_ms'][0];
                    } elseif (array_key_exists('CMD_Components_lat-SL-session_RelatedCitation_Publication_ms', $info[$key])) {
                        
                        $related_citation_publication = $info[$key]['CMD_Components_lat-SL-session_RelatedCitation_Publication_ms'][0];
                    }

                    // publisher

                    if (array_key_exists('CMD_Components_lat-corpus_RelatedCitation_Publisher_ms', $info[$key])) {

                        $related_citation_publisher = $info[$key]['CMD_Components_lat-corpus_RelatedCitation_Publisher_ms'][0];
                    } elseif (array_key_exists('CMD_Components_MPI_Collection_RelatedCitation_Publisher_ms', $info[$key])) {

                        $related_citation_publisher = $info[$key]['CMD_Components_MPI_Collection_RelatedCitation_Publisher_ms'][0];
                    } elseif (array_key_exists('CMD_Components_lat-session_RelatedCitation_Publisher_ms', $info[$key])) {
                        
                        $related_citation_publisher = $info[$key]['CMD_Components_lat-session_RelatedCitation_Publisher_ms'][0];
                    } elseif (array_key_exists('CMD_Components_MPI_Bundle_RelatedCitation_Publisher_ms', $info[$key])) {
                        
                        $related_citation_publisher = $info[$key]['CMD_Components_MPI_Bundle_RelatedCitation_Publisher_ms'][0];
                    } elseif (array_key_exists('CMD_Components_lat-SL-session_RelatedCitation_Publisher_ms', $info[$key])) {
                        
                        $related_citation_publisher = $info[$key]['CMD_Components_lat-SL-session_RelatedCitation_Publisher_ms'][0];
                    }

                    // PersistentIdentifier

                    if (array_key_exists('CMD_Components_lat-corpus_RelatedCitation_PersistentIdentifier_ms', $info[$key])) {

                        $related_citation_persistent_identifier = $info[$key]['CMD_Components_lat-corpus_RelatedCitation_PersistentIdentifier_ms'][0];
                    } elseif (array_key_exists('CMD_Components_MPI_Collection_RelatedCitation_PersistentIdentifier_ms', $info[$key])) {

                        $related_citation_persistent_identifier = $info[$key]['CMD_Components_MPI_Collection_RelatedCitation_PersistentIdentifier_ms'][0];
                    } elseif (array_key_exists('CMD_Components_lat-session_RelatedCitation_PersistentIdentifier_ms', $info[$key])) {
                        
                        $related_citation_persistent_identifier = $info[$key]['CMD_Components_lat-session_RelatedCitation_PersistentIdentifier_ms'][0];
                    } elseif (array_key_exists('CMD_Components_MPI_Bundle_RelatedCitation_PersistentIdentifier_ms', $info[$key])) {
                        
                        $related_citation_persistent_identifier = $info[$key]['CMD_Components_MPI_Bundle_RelatedCitation_PersistentIdentifier_ms'][0];
                    } elseif (array_key_exists('CMD_Components_lat-SL-session_RelatedCitation_PersistentIdentifier_ms', $info[$key])) {
                        
                        $related_citation_persistent_identifier = $info[$key]['CMD_Components_lat-SL-session_RelatedCitation_PersistentIdentifier_ms'][0];
                    }

                    // comment

                    if (array_key_exists('CMD_Components_lat-corpus_RelatedCitation_Comment_ms', $info[$key])) {

                        $related_citation_comment = $info[$key]['CMD_Components_lat-corpus_RelatedCitation_Comment_ms'][0];
                    } elseif (array_key_exists('CMD_Components_MPI_Collection_RelatedCitation_Comment_ms', $info[$key])) {

                        $related_citation_comment = $info[$key]['CMD_Components_MPI_Collection_RelatedCitation_Comment_ms'][0];
                    } elseif (array_key_exists('CMD_Components_lat-session_RelatedCitation_Comment_ms', $info[$key])) {
                        
                        $related_citation_comment = $info[$key]['CMD_Components_lat-session_RelatedCitation_Comment_ms'][0];
                    } elseif (array_key_exists('CMD_Components_MPI_Bundle_RelatedCitation_Comment_ms', $info[$key])) {
                        
                        $related_citation_comment = $info[$key]['CMD_Components_MPI_Bundle_RelatedCitation_Comment_ms'][0];
                    } elseif (array_key_exists('CMD_Components_lat-SL-session_RelatedCitation_Comment_ms', $info[$key])) {
                        
                        $related_citation_comment = $info[$key]['CMD_Components_lat-SL-session_RelatedCitation_Comment_ms'][0];
                    }

                    if (!empty($related_citation_authors)) {
                        // assuming that all necessary related citation information is present
                        break;
                    }
                }


                if ($type == "bundle" or $type == "file") {

                    // find authors(s) bottom up

                    // first see whether any overruling citation exists in the metadata up the hierarchy

                    foreach ($ancestors as $ancestor) {

                        $key = array_search($ancestor, array_column($info, 'PID'));

                        if (array_key_exists('CMD_Components_lat-corpus_Citation_Authors_ms', $info[$key])) {

                            $overruling_authors = $info[$key]['CMD_Components_lat-corpus_Citation_Authors_ms'][0];
                            $authors = $overruling_authors;
                            break;
                        } elseif (array_key_exists('CMD_Components_MPI_Collection_Citation_Authors_ms', $info[$key])) {

                            $overruling_authors = $info[$key]['CMD_Components_MPI_Collection_Citation_Authors_ms'][0];
                            $authors = $overruling_authors;
                            break;
                        }
                    }

                    if (empty($overruling_authors)) {

                        // no overruling citation, use cmd.Contributor if it exists

                        foreach ($ancestors as $ancestor) {

                            if (array_key_exists('cmd.Contributor', $info[$key])) {

                                $contributors = $info[$key]['cmd.Contributor'];
                                $authors = $this->authorsList($contributors);
                                break;
                            }
                        }
                    }

                    // find date bottom up

                    foreach ($ancestors as $ancestor) {

                        $key = array_search($ancestor, array_column($info, 'PID'));

                        if (array_key_exists('CMD_Components_lat-corpus_Citation_Date_ms', $info[$key])) {

                            $overruling_date = $info[$key]['CMD_Components_lat-corpus_Citation_Date_ms'][0];
                            $date = $overruling_date;
                            break;
                        } elseif (array_key_exists('CMD_Components_MPI_Collection_Citation_Date_ms', $info[$key])) {

                            $overruling_date = $info[$key]['CMD_Components_MPI_Collection_Citation_Date_ms'][0];
                            $date = $overruling_date;
                            break;
                        }
                    }

                    if (empty($overruling_date)) {

                        foreach ($ancestors as $ancestor) {

                            $key = array_search($ancestor, array_column($info, 'PID'));

                            if (array_key_exists('cmd.CreationDate', $info[$key])) {

                                $date_value = $info[$key]['cmd.CreationDate'][0];
                                if (($date_value == "Unspecified") or ($date_value == "Unknown")) {
                                    $date = "Date unknown";
                                } else {
                                    // only the year
                                    $date = substr($date_value, 0, 4);
                                }
                                break;
                            }
                        }
                    }
                }

                if ($type == "file") {

                    $key = array_search($pid, array_column($info, 'PID'));
                    // handle pid is one of several dc.identifier values

                    if (array_key_exists('dc.identifier', $info[$key])) {

                        $identifiers = $info[$key]['dc.identifier'];

                        foreach ($identifiers as $identifier) {
                            if (strpos($identifier, "https://hdl.handle.net") === 0) {
                                $url = $identifier;
                            }
                        }
                    }

                    // separate info query for bundle, as that is not in ancestors but in "RELS_EXT_isConstituentOf_uri_ms"
                    // in most cases the info will be available in the bunlde, however the bottom-up queries above would
                    // still be used as well

                    if (array_key_exists('RELS_EXT_isConstituentOf_uri_ms', $info[$key])) {

                        $parent = $info[$key]['RELS_EXT_isConstituentOf_uri_ms'];

                        // get rid of info:fedora prefix
                        $parent = str_replace("info:fedora/", "", $parent);

                        $parentinfo = $this->ancestorInfo($parent);

                        $parentkey = array_search($parent, array_column($info, 'PID'));

                        // get parent (=bundle) title
                        // first see whether any overruling citation exists in the metadata at the bundle level
                        if (array_key_exists('CMD_Components_lat-session_Citation_Title_ms', $parentinfo[$parentkey])) {

                            $bundle_title = $parentinfo[$parentkey]['CMD_Components_lat-session_Citation_Title_ms'][0];
                        } elseif (array_key_exists('CMD_Components_MPI_Bundle_Citation_Title_ms', $parentinfo[$parentkey])) {

                            $bundle_title = $parentinfo[$parentkey]['CMD_Components_MPI_Bundle_Citation_Title_ms'][0];
                        } elseif (array_key_exists('CMD_Components_lat-SL-session_Citation_Title_ms', $parentinfo[$parentkey])) {

                            $bundle_title = $parentinfo[$parentkey]['CMD_Components_lat-SL-session_Citation_Title_ms'][0];
                        } elseif (array_key_exists('dc.title', $parentinfo[$parentkey])) {

                            // no overruling citation, dc.title should always be there
                            $bundle_title = $parentinfo[$parentkey]['dc.title'][0];
                        }

                        // get parent date
                        // first see whether any overruling citation exists in the metadata at the bundle level
                        if (array_key_exists('CMD_Components_lat-session_Citation_Date_ms', $parentinfo[$parentkey])) {

                            $overruling_date = $parentinfo[$parentkey]['CMD_Components_lat-session_Citation_Date_ms'][0];
                            $date = $overruling_date;
                        } elseif (array_key_exists('CMD_Components_lat-session_Citation_Date_ms', $parentinfo[$parentkey])) {

                            $overruling_date = $parentinfo[$parentkey]['CMD_Components_lat-session_Citation_Date_ms'][0];
                            $date = $overruling_date;
                        } elseif (array_key_exists('CMD_Components_lat-session_Citation_Date_ms', $parentinfo[$parentkey])) {

                            $overruling_date = $parentinfo[$parentkey]['CMD_Components_lat-session_Citation_Date_ms'][0];
                            $date = $overruling_date;
                        }

                        if (empty($overruling_date)) {

                            // no overruling citation in bundle or higher collections. dc.CreationDate value should always be there but could be "Unspecified" or "Unknown"
                            if (array_key_exists('cmd.CreationDate', $parentinfo[$parentkey])) {

                                $date_value = $parentinfo[$parentkey]['cmd.CreationDate'][0];
                                if (($date_value == "Unspecified") or ($date_value == "Unknown")) {
                                    $date = "Date unknown";
                                } else {
                                    // only the year
                                    $date = substr($date_value, 0, 4);
                                }
                            }
                        }

                        // get parent authors
                        // first see whether any overruling citation exists in the metadata at the bundle level
                        if (array_key_exists('CMD_Components_lat-session_Citation_Authors_ms', $parentinfo[$parentkey])) {

                            $overruling_authors = $parentinfo[$parentkey]['CMD_Components_lat-session_Citation_Authors_ms'][0];
                        } elseif (array_key_exists('CMD_Components_MPI_Bundle_Citation_Authors_ms', $parentinfo[$parentkey])) {

                            $overruling_authors = $parentinfo[$parentkey]['CMD_Components_MPI_Bundle_Citation_Authors_ms'][0];
                        } elseif (array_key_exists('CMD_Components_lat-SL-session_Citation_Authors_ms', $parentinfo[$parentkey])) {

                            $overruling_authors = $parentinfo[$parentkey]['CMD_Components_lat-SL-session_Citation_Authors_ms'][0];
                        }


                        if (empty($overruling_authors)) {

                            // no overruling citation in bundle or higher collections, use cmd.Contributor in case it exists
                            if (array_key_exists('cmd.Contributor', $parentinfo[$parentkey])) {

                                $contributors = $parentinfo[$parentkey]['cmd.Contributor'];
                                $authors = $this->authorsList($contributors);
                            }
                        }

                        // get parent comment

                        if (array_key_exists('CMD_Components_lat-session_Citation_Comment_ms', $parentinfo[$parentkey])) {

                            $comment = $parentinfo[$parentkey]['CMD_Components_lat-session_Citation_Comment_ms'][0];
                        } elseif (array_key_exists('CMD_Components_MPI_Bundle_Citation_Comment_ms', $parentinfo[$parentkey])) {

                            $comment = $parentinfo[$parentkey]['CMD_Components_MPI_Bundle_Citation_Comment_ms'][0];
                        } elseif (array_key_exists('CMD_Components_lat-SL-session_Citation_Comment_ms', $parentinfo[$parentkey])) {

                            $comment = $parentinfo[$parentkey]['CMD_Components_lat-SL-session_Citation_Comment_ms'][0];
                        }

                        // get parent related citation

                        // authors

                        if (array_key_exists('CMD_Components_lat-session_RelatedCitation_Authors_ms', $parentinfo[$parentkey])) {

                            $related_citation_authors = $parentinfo[$parentkey]['CMD_Components_lat-session_RelatedCitation_Authors_ms'][0];
                        } elseif (array_key_exists('CMD_Components_MPI_Bundle_RelatedCitation_Authors_ms', $parentinfo[$parentkey])) {

                            $related_citation_authors = $parentinfo[$parentkey]['CMD_Components_MPI_Bundle_RelatedCitation_Authors_ms'][0];
                        } elseif (array_key_exists('CMD_Components_lat-SL-session_RelatedCitation_Authors_ms', $parentinfo[$parentkey])) {

                            $related_citation_authors = $parentinfo[$parentkey]['CMD_Components_lat-SL-session_RelatedCitation_Authors_ms'][0];
                        }

                        // date

                        if (array_key_exists('CMD_Components_lat-session_RelatedCitation_Date_ms', $parentinfo[$parentkey])) {

                            $related_citation_date = $parentinfo[$parentkey]['CMD_Components_lat-session_RelatedCitation_Date_ms'][0];
                        } elseif (array_key_exists('CMD_Components_MPI_Bundle_RelatedCitation_Date_ms', $parentinfo[$parentkey])) {

                            $related_citation_date = $parentinfo[$parentkey]['CMD_Components_MPI_Bundle_RelatedCitation_Date_ms'][0];
                        } elseif (array_key_exists('CMD_Components_lat-SL-session_RelatedCitation_Date_ms', $parentinfo[$parentkey])) {

                            $related_citation_date = $parentinfo[$parentkey]['CMD_Components_lat-SL-session_RelatedCitation_Date_ms'][0];
                        }

                        // title

                        if (array_key_exists('CMD_Components_lat-session_RelatedCitation_Title_ms', $parentinfo[$parentkey])) {

                            $related_citation_title = $parentinfo[$parentkey]['CMD_Components_lat-session_RelatedCitation_Title_ms'][0];
                        } elseif (array_key_exists('CMD_Components_MPI_Bundle_RelatedCitation_Title_ms', $parentinfo[$parentkey])) {

                            $related_citation_title = $parentinfo[$parentkey]['CMD_Components_MPI_Bundle_RelatedCitation_Title_ms'][0];
                        } elseif (array_key_exists('CMD_Components_lat-SL-session_RelatedCitation_Title_ms', $parentinfo[$parentkey])) {

                            $related_citation_title = $parentinfo[$parentkey]['CMD_Components_lat-SL-session_RelatedCitation_Title_ms'][0];
                        }

                        // publication

                        if (array_key_exists('CMD_Components_lat-session_RelatedCitation_Publication_ms', $parentinfo[$parentkey])) {

                            $related_citation_publication = $parentinfo[$parentkey]['CMD_Components_lat-session_RelatedCitation_Publication_ms'][0];
                        } elseif (array_key_exists('CMD_Components_MPI_Bundle_RelatedCitation_Publication_ms', $parentinfo[$parentkey])) {

                            $related_citation_publication = $parentinfo[$parentkey]['CMD_Components_MPI_Bundle_RelatedCitation_Publication_ms'][0];
                        } elseif (array_key_exists('CMD_Components_lat-SL-session_RelatedCitation_Publication_ms', $parentinfo[$parentkey])) {

                            $related_citation_publication = $parentinfo[$parentkey]['CMD_Components_lat-SL-session_RelatedCitation_Publication_ms'][0];
                        }

                        // publisher

                        if (array_key_exists('CMD_Components_lat-session_RelatedCitation_Publisher_ms', $parentinfo[$parentkey])) {

                            $related_citation_publisher = $parentinfo[$parentkey]['CMD_Components_lat-session_RelatedCitation_Publisher_ms'][0];
                        } elseif (array_key_exists('CMD_Components_MPI_Bundle_RelatedCitation_Publisher_ms', $parentinfo[$parentkey])) {

                            $related_citation_publisher = $parentinfo[$parentkey]['CMD_Components_MPI_Bundle_RelatedCitation_Publisher_ms'][0];
                        } elseif (array_key_exists('CMD_Components_lat-SL-session_RelatedCitation_Publisher_ms', $parentinfo[$parentkey])) {

                            $related_citation_publisher = $parentinfo[$parentkey]['CMD_Components_lat-SL-session_RelatedCitation_Publisher_ms'][0];
                        }

                        // comment

                        if (array_key_exists('CMD_Components_lat-session_RelatedCitation_Comment_ms', $parentinfo[$parentkey])) {

                            $related_citation_comment = $parentinfo[$parentkey]['CMD_Components_lat-session_RelatedCitation_Comment_ms'][0];
                        } elseif (array_key_exists('CMD_Components_MPI_Bundle_RelatedCitation_Comment_ms', $parentinfo[$parentkey])) {

                            $related_citation_comment = $parentinfo[$parentkey]['CMD_Components_MPI_Bundle_RelatedCitation_Comment_ms'][0];
                        } elseif (array_key_exists('CMD_Components_lat-SL-session_RelatedCitation_Comment_ms', $parentinfo[$parentkey])) {

                            $related_citation_comment = $parentinfo[$parentkey]['CMD_Components_lat-SL-session_RelatedCitation_Comment_ms'][0];
                        }

                        // persistent identifier

                        if (array_key_exists('CMD_Components_lat-session_RelatedCitation_PersistentIdentifier_ms', $parentinfo[$parentkey])) {

                            $related_citation_persistent_identifier = $parentinfo[$parentkey]['CMD_Components_lat-session_RelatedCitation_PersistentIdentifier_ms'][0];
                        } elseif (array_key_exists('CMD_Components_MPI_Bundle_RelatedCitation_PersistentIdentifier_ms', $parentinfo[$parentkey])) {

                            $related_citation_persistent_identifier = $parentinfo[$parentkey]['CMD_Components_MPI_Bundle_RelatedCitation_PersistentIdentifier_ms'][0];
                        } elseif (array_key_exists('CMD_Components_lat-SL-session_RelatedCitation_PersistentIdentifier_ms', $parentinfo[$parentkey])) {

                            $related_citation_persistent_identifier = $parentinfo[$parentkey]['CMD_Components_lat-SL-session_RelatedCitation_PersistentIdentifier_ms'][0];
                        }
                    }
                }

                if ($type == "collection") {

                    // check wether it is the top of the collection, i.e. no ancestor has a cmd.url

                    $topcollection = true;

                    $ancerstors_except_current = array_slice($ancestors, 1);

                    foreach ($ancerstors_except_current as $ancestor) {

                        $key = array_search($ancestor, array_column($info, 'PID'));

                        if (array_key_exists('cmd.url', $info[$key])) {

                            $topcollection = false;
                        }
                    }

                    if ($topcollection == true) {
                        $type = "top-collection";
                    }

                    // first see whether any overruling citation exists in the metadata up the hierarchy

                    // authors
                    foreach ($ancestors as $ancestor) {

                        $key = array_search($ancestor, array_column($info, 'PID'));

                        if (array_key_exists('CMD_Components_lat-corpus_Citation_Authors_ms', $info[$key])) {

                            $overruling_authors = $info[$key]['CMD_Components_lat-corpus_Citation_Authors_ms'][0];
                            $authors = $overruling_authors;
                            break;
                        } elseif (array_key_exists('CMD_Components_MPI_Collection_Citation_Authors_ms', $info[$key])) {

                            $overruling_authors = $info[$key]['CMD_Components_MPI_Collection_Citation_Authors_ms'][0];
                            $authors = $overruling_authors;
                            break;
                        }
                    }

                    // dates
                    foreach ($ancestors as $ancestor) {

                        $key = array_search($ancestor, array_column($info, 'PID'));

                        if (array_key_exists('CMD_Components_lat-corpus_Citation_Date_ms', $info[$key])) {

                            $overruling_date = $info[$key]['CMD_Components_lat-corpus_Citation_Date_ms'][0];
                            $date = $overruling_date;
                            break;
                        } elseif (array_key_exists('CMD_Components_MPI_Collection_Citation_Date_ms', $info[$key])) {

                            $overruling_date = $info[$key]['CMD_Components_MPI_Collection_Citation_Date_ms'][0];
                            $date = $overruling_date;
                            break;
                        }
                    }

                    if (empty($date) or empty($overruling_authors)) {

                        // no overruling citation for date or authors, find contributor(s) and date(s) with a facet query for everything within the selected collection

                        $query_values[] = format_string('ancestors_ms:("info:fedora/!pid" OR "!pid")', [
                            '!pid' => $pid,
                        ]);

                        $solrParams['fl'][] = 'PID';
                        $solrParams['facet'][] = 'true';
                        $solrParams['facet.mincount'][] = '1';
                        $solrParams['facet.field'][] = 'cmd.Contributor';
                        $solrParams['facet.field'][] = 'cmd.CreationDate';


                        $raw_result = $this->query($query_values, $solrParams);

                        $contributors = array_keys($raw_result['facet_counts']['facet_fields']['cmd.Contributor']);
                        $date_values = array_keys($raw_result['facet_counts']['facet_fields']['cmd.CreationDate']);
                        if (!empty($date_values) and empty($date)) {
                            $date = $this->dateRange($date_values);
                        }

                        if (!empty($contributors) and empty($overruling_authors)) {
                            $authors = $this->authorsList($contributors);
                        }
                    }
                }

                if ($type !== "top-collection") {

                    // find the highest ancestor with a cmd.url, this is the top of the collection, then get its title

                    $ancestors_top_down = array_reverse($ancestors);

                    foreach ($ancestors_top_down as $ancestor) {

                        $key = array_search($ancestor, array_column($info, 'PID'));

                        if (array_key_exists('cmd.url', $info[$key])) {

                            // first see whether any overruling citation exists in the metadata
                            if (array_key_exists('CMD_Components_lat-corpus_Citation_Title_ms', $info[$key])) {

                                $collection_title = $info[$key]['CMD_Components_lat-corpus_Citation_Title_ms'][0];
                                break;
                            } elseif (array_key_exists('CMD_Components_MPI_Collection_Citation_Title_ms', $info[$key])) {

                                $collection_title = $info[$key]['CMD_Components_MPI_Collection_Citation_Title_ms'][0];
                                break;
                            } elseif (array_key_exists('dc.title', $info[$key])) {

                                // no overruling citation, take dc.title which should always be there
                                $collection_title = $info[$key]['dc.title'][0];
                                break;
                            }
                        }
                    }
                }

                $site_name = variable_get('site_name', "Default site name");

                // if there is not enough info, return the no_citation_message rather than the partial citation.
                if (($type == "top-collection") and (empty($date) or empty($title) or empty($url))) {

                    $result .= $no_citation_message;
                } elseif (($type !== "top-collection") and (empty($date) or empty($title) or empty($collection_title) or empty($url))) {

                    $result .= $no_citation_message;
                } else {

                    // prepare citation output

                    switch ($type) {

                        case "file":
                            $object = "File<i> \"" . $title . "\"</i> in collection \"" . $collection_title . "\", bundle \"" . $bundle_title . "\"";
                            break;
                        case "bundle":
                        case "collection":
                            $object = "Item<i> \"" . $title . "\"</i> in collection \"" . $collection_title . "\"";
                            break;
                        case "top-collection":
                            $object = "Collection<i> \"" . $title . "\"</i>";
                            break;
                    }

                    if ($authors == "[author(s)]") {
                        $message .= " Author information could not be extracted automatically for this resource.";
                    }

                    $result .= "<div class=\"well citation\"><p class=\"citation\">" . $authors . " (" . $date . "). " . $object . ". " . $site_name . ". <a href=" . $url . " alt=\"Persistent identifier for object " .
                        $title . "\">" . $url . "</a>. (Accessed " . $today . ")</p>";

                    if (!empty($comment)) {
                        $result .= "<p class=\"comment\">" . $comment . "</p>";
                    }

                    if (empty($overruling_authors) and empty($overruling_date)) {

                        $result .= "<p><i>Note: " . $message . "</i></p>";
                    }

                    // prepare related citation output

                    // publication may include editors and page numbers, which should not be in italics
                    if (!empty($related_citation_publication)) {
                        $parts = preg_split('/\(Eds*\.?\)|\(pp\..+?\)/', $related_citation_publication);
                        if (!empty($parts)) {
                            // look for (Ed.) or (Eds.)
                            preg_match('/\(Eds*\.?\)/', $related_citation_publication, $eds);
                            // look for (pp.)
                            preg_match('/\(pp\..+?\)/', $related_citation_publication, $pp);
                            if (!empty($eds)) {
                                // editors found
                                $editors = $parts[0] . $eds[0];
                                $title = $parts[1];
                                $output = $editors . '<i>' . $title . '</i>';
                            }
                            elseif (!empty($pp)) {
                                // no editors found but pages found
                                $title = $parts[0];
                                $output = '<i>' . $title . '</i>' . ' ' . $pp[0];
                            }
                            if (!empty($eds) and !empty($pp)) {
                                // editors and pages found
                                $output .= ' ' . $pp[0];
                            }
                            $related_citation_publication = $output;
                        }

                        // add a period at the end if not already there
                        if (!preg_match('/\.$/', $related_citation_publication)){
                            $related_citation_publication .= '.';
                        }

                    }

                    if (!empty($related_citation_authors) and !empty($related_citation_title) and !empty($related_citation_date) and !empty($related_citation_publication) and !empty($related_citation_publisher)) {

                        $related_citation = $related_citation_authors . " (" . $related_citation_date . "). " . $related_citation_title . ". " . $related_citation_publication . " " . $related_citation_publisher . ". ";
                    } elseif (!empty($related_citation_authors) and !empty($related_citation_title) and !empty($related_citation_date) and !empty($related_citation_publisher)) {

                        $related_citation = $related_citation_authors . " (" . $related_citation_date . "). <i>" . $related_citation_title . "</i>. " . $related_citation_publisher . ". ";
                    }

                    if (!empty($related_citation_persistent_identifier)) {

                        $related_citation_persistent_identifier = $this->pidToLink($related_citation_persistent_identifier);

                        $related_citation .= $related_citation_persistent_identifier;
                    }

                    if (!empty($related_citation)) {

                        $result .= "<p><i>In addition, please cite the following publication when using this resource:</i></p>";

                        $result .= "<p class=\"citation\">" . $related_citation . "</p>";
                    }

                    if (!empty($related_citation_comment)) {
                        $result .= "<p class=\"comment\">" . $related_citation_comment . "</p>";
                    }

                    $result .= "</div>";
                }
            } else {

                // info is somehow empty
                //$result .= "<div class=\"flat-citation\"><p>" . $no_citation_message . "</p></div>";
                $result = "";
            }

            return $result;
        }
    }
}
