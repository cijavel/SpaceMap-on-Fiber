The SpaceMap shows the open status of the Hackerspaces on an LED strip.
A json is loaded from https://spaceapi.ccc.de/api/spaces and parsed for this purpose.

function:

- download the json file
- parse json to get the opening status
- customize the led strip to display the status. Each pixel is a space
- status is displayed on a local web page

