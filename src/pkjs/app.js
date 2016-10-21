
var myAPIKey = "723f29db5ad866a74f31ca638535b50f";

var customFunc = function() {
  var Clay = this;

  function toggleMuteFunc() {
    if (this.get()) {
      Clay.getItemByAppKey('CONFIG_BUZZ_MUTE').enable();
    } else {
      Clay.getItemByAppKey('CONFIG_BUZZ_MUTE').disable();
    }
  }

  Clay.on(Clay.EVENTS.AFTER_BUILD, function() {
    var toggle = Clay.getItemByAppKey('CONFIG_BUZZ');
    toggleMuteFunc.call(toggle);
    toggle.on('change', toggleMuteFunc);
  });
}

  var Clay = require('pebble-clay');
  var clayConfig = require('./config');
  var clay = new Clay(clayConfig, customFunc);

function getForecast(lat, lon) {              
  // Asking for forecast temps
  console.log('Doing forecast');

  var req = new XMLHttpRequest();
  var url = 'http://api.openweathermap.org/data/2.5/forecast/daily?lat=' + lat + '&lon=' + lon + '&cnt=1' +
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
        var min_temp = Number(response.list[0].temp.min*10-2731.5).toFixed(0);
        var max_temp = Number(response.list[0].temp.max*10-2731.5).toFixed(0);
        var dict = { TEMP_FORE_MIN: Number(min_temp),
                    TEMP_FORE_MAX: Number(max_temp)};
        Pebble.sendAppMessage(dict);
        console.log('forecast temps: ' + min_temp + ' ' + max_temp );
      } else { 
        console.log('Error'); 
      }
    }
  };
  console.log('Sending url: ' + url);
  req.send(null); 
}

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
        var temp = Number(response.main.temp*10-2731.5).toFixed(0);
        var dict = { TEMP: Number(temp)};
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

function locationForecast(pos) {
  console.log('lat= ' + pos.coords.latitude + ' lon= ' + pos.coords.longitude);
  getForecast(pos.coords.latitude, pos.coords.longitude);
}

function locationError(err) {
  console.log('location error (' + err.code + '): ' + err.message);
}

var updateWeather = function() {
  navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
};

var updateForecast = function() {
  navigator.geolocation.getCurrentPosition(locationForecast, locationError, locationOptions);
};


Pebble.addEventListener('ready', function(e) {
  console.log('JavaScript app ready and running!');
    
    // Notify the watchapp that it is now safe to send messages
  var dict = { AppKeyReady: Number(1) };
  Pebble.sendAppMessage(dict);
});

Pebble.addEventListener('appmessage', function(e) {
  console.log('AppMessage received! '+ JSON.stringify(e.payload));
  if (e.payload.REFRESH == 1) {
    console.log('Got weather request from watch');
    updateWeather();
  }
  if (e.payload.REFRESH == 2) {
    console.log('Got weather request from watch');
    updateForecast();
  }
});

// Pebble.addEventListener('showConfiguration', function(e) {
//   // Show config page
//   Pebble.openURL('http://cban.com/~asj/pebble/timeslider/');
// });

// Pebble.addEventListener('webviewclosed', function(e) {
//   // Decode and parse config data as JSON
//   var config_data = JSON.parse(decodeURIComponent(e.response));
//   console.log('Config window returned: ', JSON.stringify(config_data));

//   // Prepare AppMessage payload
//   var dict = {
//     'CONFIG_TZ_OFFSET': Number(config_data.config_tz_offset),
//     'CONFIG_METRIC': config_data.config_metric,
//     'CONFIG_BUZZ': config_data.config_buzz,
//     'CONFIG_BUZZ_MUTE': config_data.config_buzz_mute
//   };

//   // Send settings to Pebble watchapp
//   Pebble.sendAppMessage(dict, function(){
//     console.log('Sent config data to Pebble');  
//   }, function() {
//     console.log('Failed to send config data!');
//   });
// });


