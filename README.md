# qtcurl
Run **curl_easy** transfers asynchronously just inside your Qt main thread. 

Behind the scenes there is **curl_multi** legally hooked on Qt socket & timer events. Not very much of code needed for that actually. But if you're too lazy to deal with **curl_multi** in event-based system on your own, you'll probably be happy with these sources. 

Except the **curl_multi** thing now there's just only straightforward wraparound for **curl_easy** functions with a tiny bit of C++/Qt sugar. More things may be added later.


##Usage:
###1. Set up linking and includes for *libcurl*
Be strong and do it yourself. There should be plenty of docs for this.
###2. Include *qtcurl.pri* into your *.pro*
```qmake
include (qtcurl/src/qtcurl.pri)
```
###3. Write your code
Get started with including the header:
```c++
#include "CurlEasy.h"
```

Create a **CurlEasy** object and set it up:
```c++
CurlEasy *curl = new CurlEasy;
curl->set(CURLOPT_URL, "https://www.google.com");
curl->set(CURLOPT_FOLLOWLOCATION, long(1)); // Tells libcurl to follow HTTP 3xx redirects
```

You also may want to override some curl callbacks:
```c++
curl->setWriteFunction([](char *data, size_t size)->size_t {
    qDebug() << "Data from google.com: " << QByteArray(data, static_cast<int>(size));
    return size;
});
 ```

And set some HTTP headers:
```c++
curl->setHttpHeader("User-Agent", "My poor little application");
```

Oh, the signals are there, of course!
```c++
QObject::connect(curl, &CurlEasy::done, [curl](CURLcode result) {
    long httpResponseCode = curl->get<long>(CURLINFO_RESPONSE_CODE);
    QString effectiveUrl = curl->get<const char*>(CURLINFO_EFFECTIVE_URL);

    qDebug() << "Transfer for" << effectiveUrl << "is done with"
             << "HTTP" << httpResponseCode
             << "and CURL code" << result;
});
```
```c++
QObject::connect(curl, SIGNAL(done(CURLcode)),
                 curl, SLOT(deleteLater()));
```

Now it's time to activate the transfer and pass it to all those event-looping things:
```c++
curl->perform();
  
// Do your Qt stuff while the request is processing.
```

Take these usage notes into account:
- **done()** signal will NOT be emitted when the transfer is aborted by **abort()** method. **aborted()** will be emitted instead. This is a mostly convenience thing. In the most cases you don't want to do anything in **done()** when you've aborted the transfer externally.
- By default **CurlEasy** will run on the event loop of the thread from which **perform()** was called.
- Just as almost any **QObject** stuff, **CurlEasy** is not generally thread-safe. But just like other **QObject**s you can spawn it on any thread and safely use there. Even **moveToThread** is allowed for **CurlEasy** while the transfer is not running. 
- All **curl_multi**-related stuff will be created and set up on the first **CurlEasy::perform()** call in the current thread.
- If you want it to be initialized earlier, just call **CurlMulti::threadInstance()** to kick that lazy thing.
- All **curl_multi**-related stuff will be destroyed on thread exit (or **QApplication** exit, see Qt manual for **QThreadStorage**). If any related **CurlEasy** transfer has been running at this moment, it will receive **aborted()** signal.
- (Some further notes)

That's all for now. Dig into the sources for details =)
