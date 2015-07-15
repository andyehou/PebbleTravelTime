var KEY_DESTINATION = 'KEY_DESTINATION';
var DEFAULT_DESTINATION = '800 Convention Pl, Seattle, WA 98101';

var destination;


var xhrRequest = function(url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function() {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};

function locationSuccess(pos) {
  // Construct request URL to Bing Maps.
  var url = 'http://dev.virtualearth.net/REST/v1/Routes/Driving?waypoint.0=' +
      pos.coords.latitude + ',' + pos.coords.longitude +
      '&waypoint.1=' + destination +
      '&du=mi&optimize=timeWithTraffic&routeAttributes=routeSummariesOnly' +
      '&key=YOUR_BING_MAPS_API_KEY_HERE';
  
  console.log('Request URL: ' + url);
  
  // Send request.
  xhrRequest(url, 'GET', 
    function(responseText) {
      // responseText contains a JSON object with directions info.
      var json = JSON.parse(responseText);
      var dictionary = {};
      
      if (json['errorDetails']) {
        console.log('Error: ' + errorMsg);
        
        // Assemble error dictionary.
        var errorMsg = json['errorDetails'][0];
        dictionary = {
          'KEY_ERROR_MSG': errorMsg
        };
        
      } else {
        var dataObj = json['resourceSets'][0]['resources'][0];
        var duration = dataObj['travelDuration'];
        var durationInTraffic = dataObj['travelDurationTraffic'];
        console.log('Duration: ' + duration + ', in traffic: ' + durationInTraffic);

        var distance = dataObj['travelDistance'];
        console.log('Distance: ' + distance + ' mi');
        
        var condition = dataObj['trafficCongestion'];
        console.log('Condition is: ' + condition);
        
        // Assemble success dictionary.
        var dictionary = {
          'KEY_DURATION': duration,
          'KEY_DURATION_IN_TRAFFIC': durationInTraffic,
          'KEY_DISTANCE': Math.floor(distance * 10),
          'KEY_TRAFFIC_CONDITION': condition,
          'KEY_DESTINATION': destination
        };
      }

      // Send to Pebble.
      Pebble.sendAppMessage(dictionary,
        function(e) {
          console.log('Travel time info sent to Pebble successfully!');
        },
        function(e) {
          console.log('Error sending travel time info to Pebble!');
        }
      );
    }      
  );
}

function locationError(err) {
  console.log('Error requesting location!');
}

function getTravelTime() {
  navigator.geolocation.getCurrentPosition(
    locationSuccess,
    locationError,
    {timeout: 15000, maximumAge: 60000}
  );
}

// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
  function(e) {
    console.log('PebbleKit JS ready!!');
    
    // Read the destination from local storage.
    destination = localStorage.getItem(KEY_DESTINATION);
    if (!destination) {
      destination = DEFAULT_DESTINATION;
    } else {
      console.log('Got destination from local storage: ' + destination);
    }
    
    // Get the initial travel time.
    getTravelTime();
  }
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage', function(e) {
  console.log('Received appmessage!');
  getTravelTime();
});


// Listen for when settings should be opened.
Pebble.addEventListener('showConfiguration', function(e) {
  // Show config page
  Pebble.openURL('http://pushsee.com/api/travel-time/settings.html?destination=' + destination);
});


// Listen for when settings are closed.
Pebble.addEventListener('webviewclosed', function(e) {
  var configuration = JSON.parse(decodeURIComponent(e.response));
  destination = configuration['destination'];
  console.log('Configuration window returned destination: ', destination);
  
  // Save to local storage on phone.
  localStorage.setItem(KEY_DESTINATION, destination);
  console.log('Saved destination to local storage: ' + destination);
  
  getTravelTime();
});