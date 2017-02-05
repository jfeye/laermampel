const char html_index[] PROGMEM = R"=====(
  <!DOCTYPE HTML>
  <html>
  <head lang="de">
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Laermampel</title>
    <script>
      function httpGetAsync(theUrl){
          var xmlHttp = new XMLHttpRequest();
          xmlHttp.open("GET", theUrl, false); // true for asynchronous
          xmlHttp.send(null);
      }
      function setBrightness(brightness) {
        httpGetAsync("set?brightness=" + brightness);
      }
      function setSensitivity(sensitivity) {
        httpGetAsync("set?sensitivity=" + sensitivity);
      }
    </script>
    <style>
      * {
        font-family: sans-serif;
        font-size: 20pt;
      }
    </style>
  </head>
  <body>

  <p>
    <button onclick="setBrightness(0)">OFF</button>
    <button onclick="setBrightness(document.getElementById('brightness').value)">ON</button>
    <fieldset align="center">
      <legend>Brightness</legend>
      <input type="range" min="0" max="255" id="brightness" onchange="setBrightness(this.value)">
    </fieldset>
    <fieldset align="center">
      <legend>Mic Sensitivity</legend>
      <input type="range" min="0" max="255" id="sensitivity" onchange="setSensitivity(this.value)">
    </fieldset>
  </p>

  </body>
  </html>

)=====";
