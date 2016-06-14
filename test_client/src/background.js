goog.require('GoogleSmartCard.MessageChannelPool');
goog.require('GoogleSmartCard.SingleMessageBasedChannel');
goog.require('goog.asserts');
goog.require('goog.log');

console.log('HELLO THERE! test_client');

var GSC = GoogleSmartCard;

var SERVER_APP_ID = 'fbdamagffoamajjcpholpaajodfcjfim';
var CLIENT_APP_ID = 'pncffcfdfencgchlihahochlpdjcoblm';

function logTime() { console.log(new Date().toLocaleString()); }

var pool = new GSC.MessageChannelPool();
chrome.runtime.onMessageExternal.addListener(function(message, sender) {
  console.log(message, sender);
  goog.asserts.assertString(sender.id);
  var channel = pool.getChannel(sender.id);
  if (!channel) {
    channel = new GSC.SingleMessageBasedChannel(sender.id);
    pool.addChannel(channel, sender.id);
    // call something on the newly created channel
  }
  channel.deliverMessage(message);
});
chrome.runtime.onMessage.addListener(function(message, b, c) {
  console.log(message, b, c);
});

// pool.createChannel(SERVER_APP_ID)
//     .addOnDisposeCallback( function() {console.log('server channel disposed');} );
// channel.send('sname', {request_id: 'test1'});
// channel.send('sname', {sender_id: 'client2', request_id: 'test1'});

//var clientChannel = pool.createChannel(CLIENT_APP_ID);
//clientChannel.addOnDisposeCallback( function() {console.log('client channel disposed');} );
// clientChannel.send('test_service', 'test message');

chrome.runtime.sendMessage(undefined, 'test message client -> client');
