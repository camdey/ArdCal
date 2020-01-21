function doGet() {
  var calendarId = ''; // input calendar id here
  var optionalArgs = {
    timeMin: (new Date()).toISOString(),
    showDeleted: false,
    singleEvents: true,
    minResults: 2,
    maxResults: 5,
    orderBy: 'startTime'
  };

  var response = Calendar.Events.list(calendarId, optionalArgs);
  var events = response.items;
  var localTime = Date.now()/1000;   // convert current time to Unix time (seconds)
      localTime = Math.round(localTime); // round current time to drop decimals
  var i = 0;

  if (events.length > 0) {
      var event = events[i];
      var startUnix = timeConvert(event.start.dateTime);
    if (startUnix > localTime) {
      var when = event.start.dateTime;
      if (!when) {
        when = event.start.date;
      }

      var payload = startUnix + ";" + localTime; // create payload

    } else {
       i = i+1;
       event = events[i];
       startUnix = timeConvert(event.start.dateTime);
       var payload = startUnix + ";" + localTime; // create payload
    }

  } else {
    payload = 'No upcoming events found.';
  }
  return ContentService.createTextOutput(payload); //. send payload
}

function timeConvert(unixTime) {

    unixTime = unixTime.slice(0,19);         // format 2016-12-10T18:30:35
    unixTime = unixTime.replace(/-/g, "/");  // format 2016/12/10T18:30:35
    unixTime = unixTime.replace("T", " ");   // format 2016/12/10 18:30:35
    unixTime = new Date(unixTime);           // create date object

    unixTime = unixTime.toISOString();       // date object as ISO 8601 standard
    unixTime = Date.parse(unixTime)/1000;    // convert date object to Unix time (seconds)

  return (unixTime);
}
