# BrowserSelector
This is a program for opening URLs in different browsers according to their domain names.

The applicaiton should be registered as a default HTTP and HTTPS protocol handler, and in the config file you should define which domains should be opened by which browser (see sample INI file). After that, when you click an URL in some application BrowserSelector will launch, detect the URL hostname and send the URL to the browser specified by the config file.
