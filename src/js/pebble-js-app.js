var myAPIKey = "723f29db5ad866a74f31ca638535b50f";

function getTemp(lat, lon) {
	var req = new XMLHttpRequest();
  var url = 'http://api.openweathermap.org/data/2.5/weather?lat=' + lat + '&lon=' + lon + '&cnt=1' +
					 '&appid=' + myAPIKey;
	req.open('GET', url, true);
  req.error = function(e) {
    console.log('req.error: ' + req.readyState + ' status: ' + req.status);
  };
	req.onload = function(e) {
    console.log('onload: ' + req.readyState + ' status: ' + req.status);
	  if (req.readyState == 4 && req.status == 200) {
	  	if(req.status == 200) {
		    var response = JSON.parse(req.responseText);
        var temp = Number(response.main.temp-273.15).toFixed(0);
// 		    var icon = response.list[0].main.icon;
        var dict = { "TEMP": Number(temp)};
        Pebble.sendAppMessage(dict);
        console.log('temp: ' + temp );
		  } else { 
        console.log('Error'); 
      }
	  }
	};
  console.log('Sending url: ' + url);
	req.send(null);
}

var locationOptions = {
  enableHighAccuracy: false, 
  maximumAge: 10000, 
  timeout: 10000
};

function locationSuccess(pos) {
  console.log('lat= ' + pos.coords.latitude + ' lon= ' + pos.coords.longitude);
  getTemp(pos.coords.latitude, pos.coords.longitude);
}

function locationError(err) {
  console.log('location error (' + err.code + '): ' + err.message);
}

var updateWeather = function() {
  navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
};


Pebble.addEventListener('ready', function(e) {
  console.log('JavaScript app ready and running!');
  
    // Notify the watchapp that it is now safe to send messages
  Pebble.sendAppMessage({ 'AppKeyReady': 1 });
  
  setInterval(updateWeather, 3600000);
  updateWeather();
});

Pebble.addEventListener('appmessage', function(e) {
  console.log('AppMessage received! '+ JSON.stringify(e.payload));
  if (e.payload.REFRESH == 1) {
    updateWeather();
  }
});


