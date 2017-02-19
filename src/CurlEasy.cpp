#include "CurlEasy.h"
#include "CurlMulti.h"

CurlEasy::CurlEasy(CurlMulti *preferredMultiInterface, QObject *parent)
    : QObject(parent)
    , preferredMulti_(preferredMultiInterface)
{
    handle_ = curl_easy_init();
    Q_ASSERT(handle_ != nullptr);

    set(CURLOPT_PRIVATE, this);
    set(CURLOPT_XFERINFOFUNCTION, staticCurlXferInfoFunction);
    set(CURLOPT_XFERINFODATA, this);
}

CurlEasy::~CurlEasy()
{
    if (runningOnMulti_) {
        runningOnMulti_->removeTransfer(this);
        runningOnMulti_ = nullptr;
        isRunning_ = false;
    }

    if (handle_) {
        curl_easy_cleanup(handle_);
    }

    if (curlHttpHeaders_) {
        curl_slist_free_all(curlHttpHeaders_);
        curlHttpHeaders_ = nullptr;
    }
}

void CurlEasy::perform()
{
    if (isRunning_)
        return;

    isRunning_ = true;

    rebuildCurlHttpHeaders();

    if (preferredMulti_)
        runningOnMulti_ = preferredMulti_;
    else
        runningOnMulti_ = CurlMulti::threadInstance();

    runningOnMulti_->addTransfer(this);
}

void CurlEasy::abort()
{
    if (!isRunning_)
        return;

    runningOnMulti_->removeTransfer(this);
    runningOnMulti_ = nullptr;
    isRunning_ = false;

    emit aborted();
}

void CurlEasy::onCurlMessage(CURLMsg *message)
{
    if (message->msg == CURLMSG_DONE) {
        runningOnMulti_->removeTransfer(this);
        runningOnMulti_ = nullptr;
        isRunning_ = false;

        emit done(message->data.result);
    }
}

void CurlEasy::rebuildCurlHttpHeaders()
{
    if (!httpHeadersWereSet_)
        return;

    if (curlHttpHeaders_) {
        curl_slist_free_all(curlHttpHeaders_);
        curlHttpHeaders_ = nullptr;
    }

    for (auto it = httpHeaders_.begin(); it != httpHeaders_.end(); ++it) {
        const QString &header = it.key();
        const QByteArray &value = it.value();

        QByteArray headerString = header.toUtf8();
        headerString += ": ";
        headerString += value;
        headerString.append(char(0));

        curlHttpHeaders_ = curl_slist_append(curlHttpHeaders_, headerString.constData());
    }

    set(CURLOPT_HTTPHEADER, curlHttpHeaders_);
}

void CurlEasy::setReadFunction(const CurlEasy::DataFunction &function)
{
    readFunction_ = function;
    if (readFunction_) {
        set(CURLOPT_READFUNCTION, staticCurlReadFunction);
        set(CURLOPT_READDATA, this);
    } else {
        set(CURLOPT_READFUNCTION, nullptr);
        set(CURLOPT_READDATA, nullptr);
    }
}

void CurlEasy::setWriteFunction(const CurlEasy::DataFunction &function)
{
    writeFunction_ = function;
    if (writeFunction_) {
        set(CURLOPT_WRITEFUNCTION, staticCurlWriteFunction);
        set(CURLOPT_WRITEDATA, this);
    } else {
        set(CURLOPT_WRITEFUNCTION, nullptr);
        set(CURLOPT_WRITEDATA, nullptr);
    }
}

void CurlEasy::setHeaderFunction(const CurlEasy::DataFunction &function)
{
    headerFunction_ = function;
    if (headerFunction_) {
        set(CURLOPT_HEADERFUNCTION, staticCurlHeaderFunction);
        set(CURLOPT_HEADERDATA, this);
    } else {
        set(CURLOPT_HEADERFUNCTION, nullptr);
        set(CURLOPT_HEADERDATA, nullptr);
    }
}

void CurlEasy::setSeekFunction(const CurlEasy::SeekFunction &function)
{
    seekFunction_ = function;
    if (seekFunction_) {
        set(CURLOPT_SEEKFUNCTION, staticCurlSeekFunction);
        set(CURLOPT_SEEKDATA, this);
    } else {
        set(CURLOPT_SEEKFUNCTION, nullptr);
        set(CURLOPT_SEEKDATA, nullptr);
    }
}

size_t CurlEasy::staticCurlWriteFunction(char *data, size_t size, size_t nitems, void *easyPtr)
{
    CurlEasy *easy = static_cast<CurlEasy*>(easyPtr);
    Q_ASSERT(easy != nullptr);

    if (easy->writeFunction_)
        return easy->writeFunction_(data, size*nitems);
    else
        return  size*nitems;
}

size_t CurlEasy::staticCurlHeaderFunction(char *data, size_t size, size_t nitems, void *easyPtr)
{
    CurlEasy *easy = static_cast<CurlEasy*>(easyPtr);
    Q_ASSERT(easy != nullptr);

    if (easy->headerFunction_)
        return easy->headerFunction_(data, size*nitems);
    else
        return  size*nitems;
}

int CurlEasy::staticCurlSeekFunction(void *easyPtr, curl_off_t offset, int origin)
{
    CurlEasy *easy = static_cast<CurlEasy*>(easyPtr);
    Q_ASSERT(easy != nullptr);

    if (easy->seekFunction_)
        return easy->seekFunction_(static_cast<qint64>(offset), origin);
    else
        return CURL_SEEKFUNC_CANTSEEK;
}

size_t CurlEasy::staticCurlReadFunction(char *buffer, size_t size, size_t nitems, void *easyPtr)
{
    CurlEasy *transfer = static_cast<CurlEasy*>(easyPtr);
    Q_ASSERT(transfer != nullptr);

    if (transfer->readFunction_)
        return transfer->readFunction_(buffer, size*nitems);
    else
        return size*nitems;
}

int CurlEasy::staticCurlXferInfoFunction(void *easyPtr, curl_off_t downloadTotal, curl_off_t downloadNow, curl_off_t uploadTotal, curl_off_t uploadNow)
{
    CurlEasy *transfer = static_cast<CurlEasy*>(easyPtr);
    Q_ASSERT(transfer != nullptr);

    emit transfer->progress(static_cast<qint64>(downloadTotal), static_cast<qint64>(downloadNow),
                            static_cast<qint64>(uploadTotal), static_cast<qint64>(uploadNow));

    return 0;
}

void CurlEasy::removeHttpHeader(const QString &header)
{
    httpHeaders_.remove(header);
    httpHeadersWereSet_ = true;
}

QByteArray CurlEasy::httpHeaderRaw(const QString &header) const
{
    return httpHeaders_[header];
}

void CurlEasy::setHttpHeaderRaw(const QString &header, const QByteArray &encodedValue)
{
    httpHeaders_[header] = encodedValue;
    httpHeadersWereSet_ = true;
}

bool CurlEasy::set(CURLoption option, const QString &parameter)
    { return set(option, parameter.toUtf8().constData()); }

bool CurlEasy::set(CURLoption option, const QUrl &parameter)
    { return set(option, parameter.toEncoded().constData()); }

void CurlEasy::setHttpHeader(const QString &header, const QString &value)
    { setHttpHeaderRaw(header, QUrl::toPercentEncoding(value)); }

QString CurlEasy::httpHeader(const QString &header) const
    { return QUrl::fromPercentEncoding(httpHeaders_[header]); }

bool CurlEasy::hasHttpHeader(const QString &header) const
    { return httpHeaders_.contains(header); }
