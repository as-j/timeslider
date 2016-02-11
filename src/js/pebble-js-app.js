var myAPIKey = "723f29db5ad866a74f31ca638535b50f";


Pebble.addEventListener('ready', function(e) {
  console.log('JavaScript app ready and running!');
  
    // Notify the watchapp that it is now safe to send messages
  Pebble.sendAppMessage({ 'AppKeyReady': true });
  
  navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
});


function getTemp(lat, lon) {
	var req = new XMLHttpRequest();
	req.open('GET', 'http://api.openweathermap.org/data/2.5/weather?lat=' + lat + '&lon=' + lon + 'cnt=1'
					 + '&appid=' + myAPIKey, true);
	req.onload = function(e) {
	  if (req.readyState == 4 && req.status == 200) {
		if(req.status == 200) {
		  var response = JSON.parse(req.responseText);
		  console.log(response.list);
// 		  var temperature = response.list[0].main.temp;
// 		  var icon = response.list[0].main.icon;
// 		  Pebble.sendAppMessage({ 'temperature':temperature + '\u00B0C'});
		} else { console.log('Error'); }
	  }
	}
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
