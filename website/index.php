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
        <a class="button button6 button50" href="https://www.ko-fi.com/maxmakesmods">
          <img class="button_icon" src="kofi.png" alt="patreon logo">
          <div class="button_text">Ko-Fi</div>
        </a>
        <a class="button button6 button50" href="https://discord.gg/jujwEGf62K">
          <img class="button_icon" src="discord.png" alt="discord logo">
          <div class="button_text">Discord</div>
        </a>
        <a class="button button6 button50" href="https://patreon.com/maxmakesmods">
          <img class="button_icon" src="patreon.png" alt="patreon logo">
          <div class="button_text">Patreon</div>
        </a>
      </div>

      <div class="content_row">
        <div class="content_video">
          <iframe width="600" height="334" src="https://www.youtube.com/embed/4JhCGvUQyT4"></iframe>
        </div>
      </div>

      <div class="content_row">
        <div class="content_box">
          <p><b>ON STEAM OCT 19th/20th</b></p>
          <a style="border: none !important" href="https://store.steampowered.com/app/1908720/HalfLife_VR_Mod/">
            <img style="width: 300px" src="hlvrmod.jpg" alt="HLVR Mod Steam banner">
          </a>
          <p></p>
        </div>
      </div>

      <div class="content_row">
        <a href="https://github.com/maxvollmer/Half-Life-VR/releases/">Older versions on Github</a>
      </div>

      <div class="content_row">
        <div class="content_title"><h3>Features</h3></div>
        <div class="feature_list">
          <p>
            <ul>
              <li>Highly configurable experience</li>
              <ul>
                <li>Dedicated config tool that let's you modify almost anything to your liking</li>
                <li>Crouch via button or through real-life ducking/lying down (with configurable height)</li>
                <li>Customizable input actions (bind any controller input to any ingame command)</li>
                <li>Over 100 different settings to customize the mod</li>
              </ul>
              <li>True immersion</li>
              <ul>
                <li>Give NPCs commands via Windows backed speech recognition</li>
                <li>Control mounted guns and trains with your hands</li>
                <li>Every weapon can act as melee weapon</li>
                <li>Full finger tracking for controllers that support it</li>
                <li>Push, pull and grab boxes</li>
                <li>Grab and climb ladders</li>
                <li>Pull yourself up on ledges</li>
                <li>Pull levers, grab and rotate valves, push buttons</li>
                <li>Immersive HUD and flashlight</li>
                <li>HD and SD female scientists with all voice lines from the original game re-recorded by Katie Otten</li>
              </ul>
              <li>Quality of Life</li>
              <ul>
                <li>Supports every headset and controller compatible with SteamVR</li>
                <li>Free locomotion with a variety of options</li>
                <li>VR teleporter with support for ladders, xen jump pads and more</li>
                <li>Plenty of anti-nausea features for the light-stomached</li>
                <li>Scalable world, NPCs and weapons for people of different body heights and sizes</li>
                <li>Support for 3D sound and occlusion using FMOD</li>
                <li>Auto-crouch when moving or jumping against ducts, holes and caves</li>
                <li>Single-button long jump for easy jump&amp;run</li>
              </ul>
              <li>Limited Mod support</li>
              <ul>
                <li>Full support for any Half-Life mod that doesn't modify code or weapon models</li>
                <li>Limited support for any Half-Life mod that doesn't modify code but has custom weapon models</li>
                <li>Half-Life mods that modify code can't be supported for technical reasons</li>
              </ul>
              <li>Experience Half-Life on your terms</li>
              <ul>
                <li>HD textures and HD models or Classic mode: Play as if it was 1998 - just in VR!</li>
              </ul>
            </ul>
          </p>
        </div>
      </div>

      <div class="content_row">
        <div class="content_title"><h3>Planned Features</h3></div>
        <div class="feature_list">
          <p>
            <ul>
              <li>Improved immersive weapons</li>
              <ul>
                <li>Two-handed weapon handling</li>
                <li>Manual reloading</li>
                <li>Gunstock configuration</li>
                <li>Virtual Backpack</li>
                <li>Weapon selection wheel</li>
                <li>Improved grenade throwing</li>
              </ul>
              <li>Improved accessibility</li>
              <ul>
                <li>Option to reduce amount of beam effects</li>
                <li>Support for play without controllers</li>
                <li>Configurable damage feedback strength</li>
                <li>Subtitles</li>
              </ul>
              <li>And more...</li>
            </ul>
          </p>
        </div>
      </div>

      <div class="content_row">
        <div class="content_title"><h3>Known Bugs</h3></div>
        <div class="feature_list">
          <p>
            <ul>
              <li>Teleport doesn't work on some Xen jump pads</li>
              <li>Train controls are sometimes in a weird location</li>
              <li>Ladders in tight spaces are hard to climb</li>
              <li>"Rapid fire bug" (Visual glitch when shooting through walls)</li>
              <li>Teleporting on ladders sometimes bugs out</li>
              <li>HUD text is rendered in the world instead of the HUD</li>
              <li>FMOD sound stops playing after prolonged play
			    <ul>
			      <li>Quick fix: Disabling and re-enabling FMOD solves this</li>
				</ul>
			  </li>
              <li>There is a rare crash bug in the engine during save or levelchanges</li>
              <li>Some achievements don't work right</li>
              <li>Several minor visual glitches</li>
            </ul>
          </p>
        </div>
      </div>

      <div class="content_row">
        <div class="content_title"><h3>FAQ</h3></div>
        <div class="faq">
          For further questions, or to file Bug Reports, please join my Discord <a href="https://discord.gg/jujwEGf62K">Max Makes Mods</a>.
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