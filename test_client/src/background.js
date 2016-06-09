goog.require('GoogleSmartCard.MessageDispatcher');
goog.require('goog.log');

console.log('HELLO THERE! test_client');

var GSC = GoogleSmartCard;

var SERVER_APP_ID = 'fbdamagffoamajjcpholpaajodfcjfim';
var CLIENT_APP_ID = 'pncffcfdfencgchlihahochlpdjcoblm';

function logTime() { console.log(new Date().toLocaleString()); }

var messageDispatcher = new GSC.MessageDispatcher();
chrome.runtime.onMessageExternal.addListener(function(message, sender) {
  console.log(message, sender);
  var channel = messageDispatcher.getChannel(sender.id);
  if (!channel) {
    channel = messageDispatcher.createChannel(sender.id);
    // call something on the newly created channel
  }
  channel.deliverMessage(message);
});
chrome.runtime.onMessage.addListener(function(message, b, c) {
  console.log(message, b, c);
});

// messageDispatcher.createChannel(SERVER_APP_ID)
//     .addOnDisposeCallback( function() {console.log('server channel disposed');} );
// channel.send('sname', {request_id: 'test1'});
// channel.send('sname', {sender_id: 'client2', request_id: 'test1'});

var clientChannel = messageDispatcher.createChannel(CLIENT_APP_ID);
clientChannel.addOnDisposeCallback( function() {console.log('client channel disposed');} );
// clientChannel.send('test_service', 'test message');

chrome.runtime.sendMessage(undefined, 'test message client -> client');
