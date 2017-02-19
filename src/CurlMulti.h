#ifndef CURLMULTIINTERFACE_H
#define CURLMULTIINTERFACE_H

#include <curl/curl.h>
#include <QObject>
#include <QSet>

class QTimer;
class CurlEasy;
struct CurlMultiSocket;

class CurlMulti : public QObject
{
	Q_OBJECT
public:
	explicit CurlMulti(QObject *parent = nullptr);
	virtual ~CurlMulti();

	static CurlMulti* threadInstance();

	void addTransfer(CurlEasy *transfer);
	void removeTransfer(CurlEasy *transfer);
	void abortTransfer(CurlEasy *transfer);

protected slots:
	void curlMultiTimeout();
	void socketReadyRead(int socketDescriptor);
	void socketReadyWrite(int socketDescriptor);
	void socketException(int socketDescriptor);

protected:
	void curlSocketAction(curl_socket_t socketDescriptor, int eventsBitmask);
	int curlTimerFunction(int timeoutMsec);
	int curlSocketFunction(CURL *easyHandle, curl_socket_t socketDescriptor, int what, CurlMultiSocket *socket);
	static int staticCurlTimerFunction(CURLM *multiHandle, long timeoutMs, void *userp);
	static int staticCurlSocketFunction(CURL *easyHandle, curl_socket_t socketDescriptor, int what, void *userp, void *sockp);

	QTimer *timer_ = nullptr;
	CURLM *handle_ = nullptr;

	QSet<CurlEasy*> transfers_;
};

#endif // CURLMULTIINTERFACE_H
