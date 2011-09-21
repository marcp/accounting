<?php

function accounting_regions() {
  $regions = array(
       'left' => t('left sidebar'),
       'right' => t('right sidebar'),
       'content' => t('content'),
       'header' => t('header'),
       'footer' => t('footer'),
       'node_above_content' => t('above node content'),
       'node_below_content' => t('below node content'),
  );

  return $regions;
}

/**
 * Sets the body-tag class attribute.
 *
 * Adds 'sidebar-left', 'sidebar-right' or 'sidebars' classes as needed.
 */
function phptemplate_body_class($sidebar_left, $sidebar_right) {
  if ($sidebar_left != '' && $sidebar_right != '') {
    $class = 'sidebars';
  }
  else {
    if ($sidebar_left != '') {
      $class = 'sidebar-left';
    }
    if ($sidebar_right != '') {
      $class = 'sidebar-right';
    }
  }

  if (isset($class)) {
    print ' class="'. $class .'"';
  }
}

/**
 * Return a themed breadcrumb trail.
 *
 * @param $breadcrumb
 *   An array containing the breadcrumb links.
 * @return a string containing the breadcrumb output.
 */
function phptemplate_breadcrumb($breadcrumb) {
  if (!empty($breadcrumb)) {
    return '<div class="breadcrumb">'. implode(' › ', $breadcrumb) .'</div>';
  }
}

/**
 * Allow themable wrapping of all comments.
 */
function phptemplate_comment_wrapper($content, $type = null) {
  static $node_type;
  if (isset($type)) $node_type = $type;

  if (!$content || $node_type == 'forum') {
    return '<div id="comments">'. $content . '</div>';
  }
  else {
    return '<div id="comments"><h2 class="comments">'. t('Comments') .'</h2>'. $content .'</div>';
  }
}

/**
 * Override or insert PHPTemplate variables into the templates.
 */
function _phptemplate_variables($hook, $vars) {
  $vars = array();

  if ($hook == 'page') {
    if ($secondary = menu_secondary_local_tasks()) {
      $output = '<span class="clear"></span>';
      $output .= "<ul class=\"tabs secondary\">\n". $secondary ."</ul>\n";
      $vars['tabs2'] = $output;
    }

    // Hook into color.module
    if (module_exists('color')) {
      _color_page_alter($vars);
    }
  }
  else if ($hook == 'node') {
    $vars['node_above_content'] = theme('blocks', 'node_above_content');
    $vars['node_below_content'] = theme('blocks', 'node_below_content');
  }

  return $vars;
}

/**
 * Returns the rendered local tasks. The default implementation renders
 * them as tabs.
 *
 * @ingroup themeable
 */
function phptemplate_menu_local_tasks() {
  $output = '';

  if ($primary = menu_primary_local_tasks()) {
    $output .= "<ul class=\"tabs primary\">\n". $primary ."</ul>\n";
  }

  return $output;
}

/**
 * Override theme_comment_block()
 */
function phptemplate_comment_block() {
  $items = array();
  foreach (comment_get_recent() as $comment) {
    $items[] = l($comment->subject, 'node/'. $comment->nid, NULL, NULL, 'comment-'. $comment->cid) .'<span class="when">'. t(' { @time ago', array('@time' => format_interval(time() - $comment->timestamp, 1))) .' } </span>';
  }
  if ($items) {
    return theme('item_list', $items);
  }
}

