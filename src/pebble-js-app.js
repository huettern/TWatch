function HTTPGET(url) {
    var req = new XMLHttpRequest();
    req.open("GET", url, false);
    req.send(null);
    return req.responseText;
}

function getWeather(position) {
    //console.log("Get locale successful");
    //console.log("Getting weather...");
    //Get weather info
    //var response = HTTPGET("http://api.openweathermap.org/data/2.5/weather?q=Niederrohrdorf,ch");
    var request = "http://api.openweathermap.org/data/2.5/weather?lat=" + 
        String(position.coords.latitude) + "&lon=" + String(position.coords.longitude);
    
    //console.log("REQ=" + request);
  
    var response = HTTPGET(request);
  
    //Convert to JSON
    var json = JSON.parse(response);
 
    //Extract the data
    var temperature = (json.main.temp - 273.15).toFixed(1);
    var location = json.name;
    var description = json.weather[0].description;
 
    //Console output to check all is working.
    //console.log("It is " + temperature + " degrees in " + location + " today!" + description);
 
    //Construct a key-value dictionary
  var dict = {"KEY_LOCATION" : location, "KEY_TEMPERATURE": temperature, "KEY_DESCRIPTION" : description};
 
    //console.log("...Done");
    //Send data to watch for display
    Pebble.sendAppMessage(dict);
}

function getLocFail(PositionError) {
    //console.log("Get Location FAILED" + PositionError);
}

Pebble.addEventListener("ready",
  function(e) {
    //App is ready to receive JS messages
    //navigator.geolocation.getCurrentPosition(getWeather, getLocFail);
    navigator.geolocation.getCurrentPosition(getWeather,
                                             getLocFail,
                                             {maximumAge:600000}); //age in ms
  }
);

Pebble.addEventListener("appmessage",
  function(e) {
    //Watch wants new data!
    
    //console.log("Msg arrived -> Request locale");
    navigator.geolocation.getCurrentPosition(getWeather,
                                             getLocFail,
                                             {maximumAge:60000}); //age in ms
    //sconsole.log("...done");
    //navigator.geolocation.getCurrentPosition(getWeather, getLocFail);
  }
);








