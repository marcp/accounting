<?php
/**
 * @file
 * Copied from the table style plugin.
 */

/**
 * Style plugin to render each item as a row in a table.
 *
 * @ingroup views_style_plugins
 */
class views_calc_table extends views_plugin_style_table {
 
  /**
   * Option definition.
   */
  function option_definition() {
    $options = parent::option_definition();

    $options['detailed_values'] = array('default' => 0);
    $options['info'] = array('default' => array());
    return $options;
  }
   
  /**
   * Render the given style.
   */
  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
    $form['#theme'] = 'views_calc_ui_table';

    $form['detailed_values'] = array(
      '#title' => t('Show details'),
      '#type' => 'select',
      '#options' => array(0 => t('Yes'), 1 => t('No')),
      '#default_value' => $this->options['detailed_values'],
      '#description' => t("Select 'Yes' to show detailed values followed by column calculations, 'No' to surpress details and show only calculated column totals."),
    );
    
    $handlers = $this->display->handler->get_handlers('field');
    $columns = $this->sanitize_columns($this->options['columns']);
        
    foreach ($columns as $field => $column) {
      $safe = str_replace(array('][', '_', ' '), '-', $field);
      $id = 'edit-style-options-columns-' . $safe;
      $form['info'][$field]['has_calc'] = array(
        '#type' => 'checkbox',
        '#title' => t('Display calculation'),
        '#default_value' => isset($this->options['info'][$field]['has_calc']) ? $this->options['info'][$field]['has_calc'] : 0,
        '#process' => array('views_process_dependency'),
        '#dependency' => array($id => array($field)),
      );
      
      $options = _views_calc_calc_options();
      $form['info'][$field]['calc'] = array(
        '#type' => 'select',
        '#options' => $options,
        '#default_value' => isset($this->options['info'][$field]['calc']) ? $this->options['info'][$field]['calc'] : array(),
        '#process' => array('views_process_dependency'),
        '#dependency' => array('edit-style-options-info-'. $safe .'-has-calc' => array(TRUE)),
        '#multiple' => TRUE,
      );
    }
  }

  /**
   * Views Method pre_render().
   *
   * Build grand total and page sub total.
   * Query calc fields using sub-view and add data.
   *
   * TODO
   * figure out what changes are needed so Views field groups will work.
   */
  function pre_render($results) {
    parent::pre_render($results);
    
    // If there are no calc fields, do nothing.
    if (!$calc_fields = $this->get_calc_fields()) {
      return;
    }
    // If we're not getting a summary row, do nothing.
    if (!empty($this->view->views_calc_calculation)) {
      return;
    }
    $this->view->totals = array();
    $this->view->sub_totals = array();
    $this->view->views_calc_fields = $calc_fields;
    $this->view->views_calc_calculation = FALSE;

    $maxitems = $this->view->get_items_per_page();

    // check if Subtotals are displayed
    if (!empty($maxitems)
    && ($this->view->query->pager->get_total_items() > $maxitems))  {
      $ids = array();
      foreach ($this->view->result as $delta => $value) {
        $ids[] = $value->{$this->view->base_field};
      }
      // Add sub_total rows to the results.
      // We need one query per aggregation because theming needs unrenamed views field alias.
      // TODO Looks like we have problems unless we
      // force a non-page display, need to keep an eye on this.
      $this->execute_summary_view($ids);
    }
    
    // Add grand totals to the results.
    $this->execute_summary_view();
  }

  function execute_summary_view($ids = array()) {
    // Clone view for local subquery.
    $summary_view = $this->view->clone_view();
    // copy the query object by value not by reference!
    $summary_view->query = drupal_clone($this->view->query);

    // Make sure the view is completely valid.
    $errors = $summary_view->validate();
    if (is_array($errors)) {
      foreach ($errors as $error) {
        drupal_set_message($error, 'error');
      }
      return NULL;
    }

    // intialize summary view
    $is_subtotal = !empty($ids);
    $summary_view->preview = TRUE;
    $summary_view->is_cacheable = FALSE;
    $summary_view->views_calc_calculation = TRUE;
    $summary_view->views_calc_sub_total = $is_subtotal;
    // If it's a subtotal calc, we're filtering the page elements by $ids.
    $summary_view->views_calc_ids = $ids;
    $summary_view->views_calc_fields = $this->view->views_calc_fields;
    // We don't need any offset in the calculation queries because
    // the statement does only return the records with the passed ids
    $summary_view->query->offset = 0;
    // Execute and render the view in preview mode. Note that we only store
    // the result if the executed view query returns any result.
    $summary_view->pre_execute($this->view->args);
    // All results of the calculation queries are used, so we don't page them.
    // Else we will loose the global pager from the page view.
    $summary_view->set_items_per_page(0);

    $summary_view->execute();
    $summary_view->post_execute();

    if (!empty($summary_view->result)) {
      if ($is_subtotal) {
        $this->view->sub_totals = array_shift($summary_view->result);
      }
      else {
        $this->view->totals = array_shift($summary_view->result);
      }
    }
  }

  /**
   * Views Method query().
   */
  function query() {
    parent::query();
    // If we're not getting a summary row, do nothing.
    if (empty($this->view->views_calc_calculation)) {
      return;
    }

    // If there are no calc fields, do nothing.
    if (!$this->view->views_calc_fields) {
      return;
    }

    // Rebuild the total query.
    $this->query_total();
  }
  
  /**
   * Query grand total
   *
   * The grand total can be computed using GROUPBY without regard to pager values.
   *
   * @see query().
   */
  function query_total() {
    // Create summary rows.
    
    // Empty out any fields that have been added to the query,
    // we don't need them for the summary totals.
    $this->view->query->fields = array();
    // Clear out any sorting and grouping, it can create unexpected results
    // when Views adds aggregation values for the sorts.
    $this->view->query->orderby = array();
    $this->view->query->groupby = array();

    $calc_fields = $this->view->views_calc_fields;
    foreach ($calc_fields as $calc => $fields) {
      foreach ($this->view->field as $field) {
        $query_field = substr($field->field, 0, 3) == 'cid' ? $field->definition['calc'] : $field->table .'.'. $field->field;
        $query_alias = $field->field_alias;
        // Bail if we have a broken handler.
        if ($query_alias == 'unknown') {
          continue;
        }
        
        $this->view->query->add_table($field->table, NULL, NULL, $field->table);
        // add all fields
        $this->view->query->add_field(NULL, "NULL", $query_alias);
        // aggregation functions
        $ext_alias = $calc.'__'.$query_alias;
        if (in_array($field->field, $fields)) {
          // Calculated fields.
          $this->view->query->add_field(NULL, $calc . '(' . $query_field . ')', $ext_alias);
        }
      }
    }
    // TODO This won't work right with relationships, need a fix here.
    // Limit to specific primary ids. This is for subtotals.
    if (!empty($this->view->views_calc_ids)) {
      $this->view->query->add_where(NULL, $this->view->base_table . "." . $this->view->base_field . " IN (%s)", implode(',', $this->view->views_calc_ids));
    }
  }

  /**
   * Get views_calc fields
   */
  function get_calc_fields() {
    // TODO on preview this returns the wrong list.
    $options  = $this->view->style_plugin->options;
    $handler  = $this->view->style_plugin;
    $fields   = $this->view->field;
    $columns  = $handler->sanitize_columns($options['columns'], $fields);
    $calcs = array_keys(_views_calc_calc_options());
    
    $calc_fields = array();
    foreach ($columns as $field => $column) {
      if ($field == $column && empty($fields[$field]->options['exclude'])) {
        if ($options['info'][$field]['has_calc']) {
          foreach ($calcs as $calc) {
            if (isset($this->options['info'][$field]['calc'][$calc])) {
              $calc_fields[$calc][] = $field;
            }
          }
        }
      }
    }
    return $calc_fields;
  }
}
