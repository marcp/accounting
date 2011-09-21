<?php phptemplate_comment_wrapper(NULL, $node->type); ?>

<div id="node-<?php print $node->nid; ?>" class="node<?php if ($sticky) { print ' sticky'; } if (!$status) { print ' node-unpublished'; } ?> ntype-<?php print $node->type; ?>">

<?php print $picture ?>

<?php if ($page == 0): ?>
  <h2><a href="<?php print $node_url ?>" title="<?php print $title ?>"><?php print $title ?></a></h2>
<?php endif; ?>

  <?php if ($submitted): ?>
    <span class="submitted">
    <span class="ntype">
<?php
    print '[ '. $node->type .' ]';
?>
    </span>
<?php
  if ($node->created == $node->changed) {
    print t('Created on !date by !username.', array('!username' => theme('username', $node), '!date' => format_date($node->created)));
  }
  else {
    print t('Created on !date by !username.  Modified on !mod_date by !mod_username.', array('!username' => theme('username', $node), '!date' => format_date($node->created), '!mod_date' => format_date($node->changed), '!mod_username' => theme('username', user_load(array('uid' => $node->revision_uid)))));
  }
?>
</span>
  <?php endif; ?>

  <div class="content">
    <?php print $node_above_content; ?>
    <div class="clear-block"><?php print $content ?></div>
    <?php print $node_below_content; ?>
  </div>

  <div class="clear-block clear">
    <div class="meta">
    <?php if ($taxonomy): ?>
      <div class="terms"><?php print $terms ?></div>
    <?php endif;?>
    </div>

    <?php if ($links): ?>
      <div class="links"><?php print $links; ?></div>
    <?php endif; ?>
  </div>

</div>
