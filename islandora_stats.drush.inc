<?php

ini_set('memory_limit', '256M');

function islandora_stats_drush_command() {
  $items['islandora_stats'] = array(
    'callback' => 'islandora_stats_get_stats',
    'description' => dt('Get stats'),
    'examples' => array(
      'drush -u 3 -l http://example.com islandora_stats'
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
  );
  return $items;
}

function islandora_stats_get_stats() {
  $xml_mimes = array('text/xml', 'application/xml');
  // query for objects
  $pids = _islandora_stats_object_query();

  $dsids = array();
  $ds_sizes = array();
  $mimes = array();
  //$xml_stats = array();

  $fp = fopen('obj_stats.csv', 'w');
  fputcsv($fp, array('Pid', 'Label', 'Datastream Count', 'Datastreams', 'Mimetypes'));

  foreach ($pids as $pid) {

    $object = islandora_object_load($pid);

    $object_datastreams = array();
    $object_mimes = array();

    foreach ($object as $ds) {
      $dsids[] = $ds->id;
      $object_datastreams[] = $ds->id;
      $mimes[] = $object_mimes[] = $ds->mimetype;

      if (in_array($ds->mimetype, $xml_mimes)) {
        // Don't bother with XML analysis for now.
        /*
        $datastream_content = $ds->getContent();

        $doc = @new SimpleXMLElement($datastream_content);

        $child_count = $doc->count();

        $xml_stats[$ds->id][] = array();
        */
      }

      $ds_sizes[$ds->id][] = $ds->size;

    }

    $ds_count = count($object_datastreams);
    sort($object_datastreams);
    sort($object_mimes);
    $ds_list = implode(', ', $object_datastreams);
    $mime_list = implode(', ', $object_mimes);

    $out = array($pid, $object->label, $ds_count, $ds_list, $mime_list);
    drush_print('Object: ' . $pid . ' Datastreams: ' . $ds_list);
    fputcsv($fp, $out);
  }

  fclose($fp);

  $fp = fopen('ds_stats.csv', 'w');

  fputcsv($fp, array('Type', 'Count'));

  $mime_counts = array_count_values($mimes);

  foreach ($mime_counts as $k => $v) {
    fputcsv($fp, array($k, $v));
  }

  fputcsv($fp, array(''));
  fputcsv($fp, array('Datastream', 'Count', 'Avg File Size'));

  $datastreams = array_count_values($dsids);

  foreach ($datastreams as $datastream => $count) {
    $avg_size = array_sum($ds_sizes[$datastream]) / $count;
    if ($avg_size == 0) {
      $avg_size = '-';
    }
    fputcsv($fp, array($datastream, $count, $avg_size));
  }

  fclose($fp);
}

function _islandora_stats_object_query() {
  module_load_include('inc', 'islandora', 'includes/tuque');

  try {
    $connection = new IslandoraTuque();

    $query = 'select $object $title from <#ri> where $object <dc:title> $title
    minus $object <mulgara:is><info:fedora/fedora-system:FedoraObject-3.0>
    minus $object <mulgara:is><info:fedora/fedora-system:ContentModel-3.0>
    minus $object <mulgara:is><info:fedora/fedora-system:ServiceDefinition-3.0>
    minus $object <mulgara:is><info:fedora/fedora-system:ServiceDeployment-3.0>
    order by $object';

    $results = $connection->repository->ri->itqlQuery($query);
  }
  catch(Exception $e) {
    drush_print($e);
  }

  $objects = array();
  foreach ($results as $model) {
    $objects[] = $model['object']['value'];
  }
  $objects = array_unique($objects);
  $objects = array_values($objects);

  return $objects;
}