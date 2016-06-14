goog.require('GoogleSmartCard.MessageChannelPool');
goog.require('GoogleSmartCard.PortMessageChannel');
goog.require('GoogleSmartCard.SingleMessageBasedChannel');
goog.require('goog.asserts');
goog.require('goog.log');

console.log('HELLO THERE! test_server');

var GSC = GoogleSmartCard;

function logTime() {
  console.log(new Date().toLocaleString());
}

//chrome.runtime.onMessageExternal.addListener(externalMessageListener);
//pool.onCreateChannel(this.createClientHandler_.bind(this)); ???
var pool = new GSC.MessageChannelPool();
chrome.runtime.onMessageExternal.addListener(function(request, sender) {
  goog.asserts.assertString(sender.id);
  var channel = pool.getChannel(sender.id);
  if (!channel) {
    channel = new GSC.SingleMessageBasedChannel(sender.id);
    pool.addChannel(channel, sender.id);
    // call something on the newly created channel
  }
  channel.deliverMessage(request);
});
