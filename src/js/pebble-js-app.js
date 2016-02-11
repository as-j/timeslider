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
	  	  console.log(response);
        var temp = response.main.temp-273.15;
        var temp_high = response.main.temp_high-273.15;
        var temp_low = response.main.temp_low-273.15;
// 		    var icon = response.list[0].main.icon;
		    Pebble.sendAppMessage({ 'temp':temp,
                               'temp_high':temp_high,
                               'temp_low':temp_low});
        console.log('temp: ' + temp);
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
  Pebble.sendAppMessage({ 'AppKeyReady': true });
  
  setInterval(updateWeather, 3600000);
  updateWeather();
});



