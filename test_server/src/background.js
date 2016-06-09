goog.require('goog.log');
goog.require('GoogleSmartCard.PortMessageChannel');
goog.require('GoogleSmartCard.MessageDispatcher');

console.log('HELLO THERE! test_server');

var GSC = GoogleSmartCard;

function logTime() {
  console.log(new Date().toLocaleString());
}

//chrome.runtime.onMessageExternal.addListener(externalMessageListener);
//messageDispatcher.onCreateChannel(this.createClientHandler_.bind(this)); ???
var messageDispatcher = new GSC.MessageDispatcher();
chrome.runtime.onMessageExternal.addListener(function(request, sender) {
  var channel = messageDispatcher.getChannel(sender.id);
  if (!channel) {
    channel = messageDispatcher.createChannel(sender.id);
    // call something on the newly created channel
  }
  channel.deliverMessage(request);
});

