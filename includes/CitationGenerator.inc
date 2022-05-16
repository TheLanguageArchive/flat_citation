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
        $data     = $this->query($this->ancestorsQuery($ancestors), $this->ancestorsParams($ancestors));

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
    public function ancestorsParams($parent)
    {

        $solrParams['fl'][] = 'PID,dc.title,cmd.Contributor,cmd.CreationDate,cmd.url,RELS_EXT_hasModel_uri_ms,RELS_EXT_isConstituentOf_uri_ms,dc.identifier';

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
        }

        else {
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
    public function authorsList($authors) {

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
        $date = "";
        $title = "";
        $bundle_title = "";
        $collection_title = "";
        $url = "";
        $type = "";
        $model = "";
        $top_collection = false;

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

                if (array_key_exists('RELS_EXT_hasModel_uri_ms', $info[$key])) {

                    $models = $info[$key]['RELS_EXT_hasModel_uri_ms'];

                    $type = "file";

                    if (in_array("info:fedora/islandora:collectionCModel", $models) AND !in_array("info:fedora/islandora:sp_cmdiCModel", $models)) {
                        // object is a higher level collection that does not have CMDI metadata, no citation needed
                        return;
                    }

                    if (in_array("info:fedora/islandora:collectionCModel", $models)) {
                        $type = "collection";
                    }

                    if (in_array("info:fedora/islandora:compoundCModel", $models) AND !in_array("info:fedora/islandora:collectionCModel", $models)) {
                        $type = "bundle";
                    }

                }

                // title and url of the current pid

                if (array_key_exists('dc.title', $info[$key])) {

                    $title = $info[$key]['dc.title'][0];

                }

                if (array_key_exists('cmd.url', $info[$key])) {

                    $url = $info[$key]['cmd.url'][0];

                }



                if ($type == "bundle" OR $type == "file") {

                    // find authors(s) bottom up

                    foreach ($ancestors as $ancestor) {

                        $key = array_search($ancestor, array_column($info, 'PID'));

                        if (array_key_exists('cmd.Contributor', $info[$key])) {

                            $contributors = $info[$key]['cmd.Contributor'];
                            $authors = $this->authorsList($contributors);
                            break;

                        }
                    }


                    // find date bottom up

                    foreach ($ancestors as $ancestor) {

                        $key = array_search($ancestor, array_column($info, 'PID'));

                        if (array_key_exists('cmd.CreationDate', $info[$key])) {

                            $date_value = $info[$key]['cmd.CreationDate'][0];
                            if (($date_value == "Unspecified") OR ($date_value == "Unknown")) {
                                $date = "Date unknown";
                            }
                            else {
                                // only the year
                                $date = substr($date_value, 0, 4);
                            }
                            break;

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

                        $parentkey = array_search($parent , array_column($info, 'PID'));

                        // get parent date
                        if (array_key_exists('cmd.CreationDate', $parentinfo[$parentkey])) {

                            $date_value = $parentinfo[$parentkey]['cmd.CreationDate'][0];
                            if (($date_value == "Unspecified") OR ($date_value == "Unknown")) {
                                $date = "Date unknown";
                            }
                            else {
                                // only the year
                                $date = substr($date_value, 0, 4);
                            }

                        }

                        // get parent title
                        if (array_key_exists('dc.title', $parentinfo[$parentkey])) {

                            $bundle_title = $parentinfo[$parentkey]['dc.title'][0];

                        }

                        // get parent authors
                        if (array_key_exists('cmd.Contributor', $parentinfo[$parentkey])) {

                            $contributors = $parentinfo[$parentkey]['cmd.Contributor'];
                            $authors = $this->authorsList($contributors);

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

                    // find contributor(s) and date(s) with a facet query for everything within the selected collection

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
                    if (!empty($date_values)) {
                        $date = $this->dateRange($date_values);
                    }

                    if (!empty($contributors)) {
                        $authors = $this->authorsList($contributors);
                    }

                }

                // find the highest ancestor with a cmd.url, this is the top of the collection, then get its dc.title

                $ancestors_top_down = array_reverse($ancestors);

                foreach ($ancestors_top_down as $ancestor) {

                    $key = array_search($ancestor, array_column($info, 'PID'));

                    if (array_key_exists('cmd.url', $info[$key])) {

                        $collection_title = $info[$key]['dc.title'][0];
                        break;

                    }

                }

                $site_name = variable_get('site_name', "Default site name");

                // if there is not enough info, return the no_citation_message rather than the partial citation.
                if (($type == "top-collection") AND (empty($date) OR empty($title) OR empty($url))) {
                    $result .= $no_citation_message;
                }

                elseif (empty($date) OR empty($title) OR empty($collection_title) OR empty($url)) {
                    $result .= $no_citation_message;
                }

                else {

                    switch($type) {

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

                    $result .= "<div class=\"well citation\"><p>" . $authors . ". (" . $date . "). " . $object . ". " . $site_name . ". <a href=" . $url. " alt=\"Persistent identifier for object " .
                    $title . "\">" . $url . "</a>. (Accessed " . $today . ")</p>";
                    $result .= "<p><i><small>Note: " . $message . "</small></i></p></div>";

                }

            }

            else {
                //$result .= "<div class=\"flat-citation\"><p>" . $no_citation_message . "</p></div>";
                $result = "";

            }

            return $result;
        }

    }

}