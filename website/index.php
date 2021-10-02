<?php
  // äöü - force UTF-8
  $title = "Half-Life: VR";
  $desc = "A mod for the original Half-Life adding support for playing the game in Virtual Reality.";
  $url = "https://www.halflifevr.de/";
  $logo = $url . "logo.png";
  $header = $url . "header.png";
  $favicon = $url . "favicon.ico";
  $impressum = $url . "impressum.html";
  $MOD_VERSION = "0.6.29-beta";
?>
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="utf-8">

    <title><?=$title?></title>

    <link rel="shortcut icon" type="image/x-icon" href="<?=$favicon?>"/>

    <meta name="original-source" content="<?=$url?>" />
    <link rel="canonical" href="<?=$url?>" />

    <meta property="og:title" content="<?=$title?>" />
    <meta property="og:site_name" content="<?=$title?>" />
    <meta property="og:description" content="<?=$desc?>" />
    <meta property="og:url" content="<?=$url?>" />
    <meta property="og:image" content="<?=$logo?>" />
    <meta property="og:image:width" content="500" />
    <meta property="og:image:height" content="500" />

    <meta name="twitter:card" content="summary" />
    <meta name="twitter:description" content="<?=$desc?>" />
    <meta name="twitter:title" content="<?=$title?>" />
    <meta name="twitter:image" content="<?=$logo?>" />

    <link href="styles.css" rel="stylesheet">
  </head>
  <body>
    <div class="header">
      <div class="header_title">
        <h1>Half-Life: VR</h1>
        <h2>A highly immersive Virtual Reality mod for the classic Half-Life from 1998.</h2>
        <p>Requires Windows PC with Steam, SteamVR and Half-Life.</p>
      </div>
      <a href="https://lambda1vr.com" class="lambda1vrbox">
        <p>Looking for <b>Lambda1VR</b>, the VR port for the Quest by DrBeef?</p>
        <p><b>Click here!</b></p>
      </a>
    </div>

    <div class="content">

      <div class="content_row">
        <a class="button" href="https://github.com/maxvollmer/Half-Life-VR">
          <img class="button_icon" src="github.png" alt="github logo">
          <div class="button_text">Source Code<br>Readme, Credits, Licensing</div>
        </a>
      </div>

      <div class="content_row">
        <div class="content_video">
          <iframe width="420" height="315" src="https://www.youtube.com/embed/tgbNymZ7vqY"></iframe>
        </div>
        <div class="subnote">Didn't finish a promotional video for the mod in time. So you get this instead.</div>
      </div>

      <div class="content_row">
        <div class="content_box">
          <p><b>NOTE</b></p>
          <p>This mod is currently released as beta. There are a bunch of planned features not yet implemented, and it probably has plenty of bugs. If you are looking for a fully fleshed out experience, don't play the mod at this point.</p>
        </div>
      </div>

      <div class="content_row">
        <div class="content_title"><h3>Community</h3></div>
        <a class="button button3" href="https://discord.gg/32VbCSt">
          <img class="button_icon" src="discord.png" alt="discord logo">
          <div class="button_text">Join our Discord server</div>
        </a>
        <a class="button button3" href="https://www.reddit.com/r/HalfLife_VR/">
          <img class="button_icon" src="reddit_logo.png" alt="reddit logo">
          <div class="button_text">Half-Life: VR subreddit</div>
        </a>
      </div>

      <div class="content_row">
        <div class="content_title">
          <h3>Downloads</h3>
        </div>
        <a class="button button2" href="https://github.com/maxvollmer/Half-Life-VR/releases/download/<?php echo $MOD_VERSION; ?>/HLVR_<?php echo $MOD_VERSION; ?>-Installer.exe">
          <div class="button_text">
            <h4>Download Installer</h4>
            <p>
              <ul>
                <li>Download</li>
                <li>Install</li>
                <li>Play</li>
              </ul>
            </p>
          </div>
        </a>
        <a class="button button2" href="https://github.com/maxvollmer/Half-Life-VR/releases/download/<?php echo $MOD_VERSION; ?>/HLVR_<?php echo $MOD_VERSION; ?>.zip">
          <div class="button_text">
            <h4>Download Archive</h4>
            <p>
              <ul>
                <li>Download</li>
                <li>Copy vr folder into Half-Life directory</li>
                <li>Play</li>
              </ul>
            </p>
          </div>
        </a>
        <br><br>
        <a href="https://github.com/maxvollmer/Half-Life-VR/releases/">Older versions</a>
      </div>

      <div class="content_row">
        <div class="content_title"><h3>Support</h3></div>
        <a class="button button6" href="https://www.patreon.com/maxvollmer">
          <img class="button_icon" src="patreon.png" alt="patreon logo" title="Support">
        </a>
        <a class="button button6" href="https://www.paypal.me/maxvollmer">
          <img class="button_icon" src="paypal.png" alt="paypal logo" title="Donate">
        </a>
      </div>

      <div class="content_row">
        <div class="content_title"><h3>Features</h3></div>
        <div class="feature_list">
          <p>
            <ul>
              <li>Dedicated HLVRConfig.exe that let's you modify almost anything to your liking</li>
              <li>Supports every headset and controller compatible with SteamVR</li>
              <li>Every weapon can act as melee weapon</li>
              <li>Free locomotion with a variety of options</li>
              <li>VR teleporter with support for ladders, xen jump pads and more</li>
              <li>Full finger tracking for controllers that support it</li>
              <li>Pull levers, grab and rotate valves, push buttons</li>
              <li>Push, pull and grab boxes</li>
              <li>Grab and climb ladders</li>
              <li>Pull yourself up on ledges</li>
              <li>Give NPCs commands via Windows backed speech recognition</li>
              <li>Immersive controls for mounted guns and controllable trains</li>
              <li>Virtual backpack</li>
              <li>Crouch via button or through real-life ducking/lying down (with configurable height)</li>
              <li>Auto-crouch when moving or jumping against ducts, holes and caves</li>
              <li>Single-button long jump for easy jump&amp;run</li>
              <li>Customizable input actions (bind any controller input to any ingame command)</li>
              <li>Optional HD textures and HD models</li>
              <li>Female scientists with all voice files from the original game in a female voice</li>
              <li>Immersive HUD (can be attached to HMD or controller)</li>
              <li>Immersive flashlight (can be attached to HMD or controller)</li>
              <li>Plenty of anti-nausea features for the light-stomached</li>
              <li>Scalable world, NPCs and weapons for people of different body heights and sizes</li>
              <li>Support for play without controllers (aim with your head, shoot with ENTER or mouse)</li>
              <li>Support for 3d sound and occlusion using FMOD</li>
              <li>Classic mode: Play as if it was 1998 - just in VR!</li>
            </ul>
          </p>
        </div>
      </div>

      <div class="content_row">
        <div class="content_title"><h3>Planned Features</h3></div>
        <div class="feature_list">
          <p>
            <ul>
              <li>Two-handed weapon handling</li>
              <li>Gunstock configuration</li>
              <li>Forward pulling with controllers while crouching</li>
              <li>Stair/step-smoothing</li>
              <li>Proper main and config menus in VR</li>
              <li>Proper teleporter arc (following path of jumping player)</li>
              <li></li>
              <li></li>
            </ul>
          </p>
        </div>
      </div>

      <div class="content_row">
        <div class="content_title"><h3>FAQ</h3></div>
        <div class="faq">
          <?php
            $faqfile = file_get_contents("faq.txt");
            $lines = explode("\n", $faqfile);
            foreach($lines as $line) {
              $q_and_a = explode("|", $line);
              $question = $q_and_a[0];
              $answer = $q_and_a[1];
              echo "<div class='faq_box'><h5>$question</h5><p>$answer</p></div>";
            }
          ?>
        </div>
      </div>

    </div>

    <div class="footer">
      <br>
      <p>Half-Life, Steam and all related IPs (c) Valve. This website is not affiliated with Valve.</p>
      <p>
        <a class="impressum_link" href="<?=$impressum?>">IMPRESSUM / IMPRINT</a>
      </p>
    </div>
  </body>
</html>