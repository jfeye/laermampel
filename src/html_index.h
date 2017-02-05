const char html_index[] PROGMEM = R"=====(
  <!DOCTYPE HTML>
  <html>
  <head lang="de">
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>OP-Leuchte</title>
    <script>
      function httpGetAsync(theUrl){
          var xmlHttp = new XMLHttpRequest();
          xmlHttp.open("GET", theUrl, false); // true for asynchronous
          xmlHttp.send(null);
      }
      function led(led, brightness){
          httpGetAsync("set?led=" + led + "&b=" + brightness);
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
    <button onclick="httpGetAsync('off')">OFF</button>
    <button onclick="httpGetAsync('on')">ON</button>
    <button onclick="httpGetAsync('rotate')">ROTATE</button>
  </p>

  <script>
    for (var i = 0; i<5; i++) {
      document.write("<p>" + i + ": ");
      for (var k = 0; k<5; k++) {
        var b = k * 255 / 5;
        document.write("<button onclick=\"led(" + i + ", " + b + ")\">" + k + "</button> ");
      }
      document.write("</p>");
    }
  </script>

  </body>
  </html>




)=====";
