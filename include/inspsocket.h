/*       +------------------------------------+
 *       | Inspire Internet Relay Chat Daemon |
 *       +------------------------------------+
 *
 *  InspIRCd: (C) 2002-2011 InspIRCd Development Team
 * See: http://wiki.inspircd.org/Credits
 *
 * This program is free but copyrighted software; see
 *	    the file COPYING for details.
 *
 * ---------------------------------------------------
 */

#ifndef __INSP_SOCKET_H__
#define __INSP_SOCKET_H__

/**
 * States which a socket may be in
 */
enum BufferedSocketState
{
	/** Socket disconnected */
	I_DISCONNECTED,
	/** Socket connecting */
	I_CONNECTING,
	/** Socket fully connected */
	I_CONNECTED,
	/** Socket has an error */
	I_ERROR
};

/**
 * Error types which a socket may exhibit
 */
enum BufferedSocketError
{
	/** No error */
	I_ERR_NONE,
	/** Socket was closed by peer */
	I_ERR_DISCONNECT,
	/** Socket connect timed out */
	I_ERR_TIMEOUT,
	/** Socket could not be created */
	I_ERR_SOCKET,
	/** Socket could not connect (refused) */
	I_ERR_CONNECT,
	/** Socket could not bind to local port/ip */
	I_ERR_BIND,
	/** Socket could not write data */
	I_ERR_WRITE,
	/** No more file descriptors left to create socket! */
	I_ERR_NOMOREFDS,
	/** Some other error */
	I_ERR_OTHER
};

/* Required forward declarations */
class BufferedSocket;

/** Private data handler and function dispatch for I/O */
class CoreExport IOHook : public classbase
{
 public:
	/** Module that is providing this service */
	ModuleRef creator;
	IOHook(Module* Creator) : creator(Creator) {}
	virtual int OnRead(StreamSocket*, std::string& recvq) = 0;
	virtual int OnWrite(StreamSocket*, std::string& sendq) = 0;
	virtual void OnClose(StreamSocket*) = 0;
};

class CoreExport IOHookProvider : public ServiceProvider
{
 public:
	IOHookProvider(Module* Creator, const std::string& Name)
		: ServiceProvider(Creator, Name, SERVICE_IOHOOK) {}
	/**
	 * Setup for oubound connection
	 * @param socket The new socket wrapper we are binding to
	 * @param tag Configuration information for this connection (may be NULL)
	 */
	virtual void OnClientConnection(StreamSocket*, ConfigTag* tag) = 0;

	/**
	 * Setup for inbound connection
	 * @param socket The new socket wrapper we are binding to
	 * @param from The port binding that this socket came from
	 */
	virtual void OnServerConnection(StreamSocket*, ListenSocket* from) = 0;
};

/**
 * StreamSocket is a class that wraps a TCP socket and handles send
 * and receive queues, including passing them to IO hooks
 */
class CoreExport StreamSocket : public EventHandler
{
	/** Module that handles raw I/O for this socket, or NULL */
	IOHook* hook;
	/** Private send queue. Note that individual strings may be shared
	 */
	std::deque<std::string> sendq;
	/** Length, in bytes, of the sendq */
	size_t sendq_len;
	/** Error - if nonempty, the socket is dead, and this is the reason. */
	std::string error;
 protected:
	std::string recvq;
 public:
	StreamSocket() : hook(NULL), sendq_len(0) {}
	inline IOHook* GetIOHook() { return hook; }
	inline void SetIOHook(IOHook* h) { hook = h; }
	/** Handle event from socket engine.
	 * This will call OnDataReady if there is *new* data in recvq
	 */
	virtual void HandleEvent(EventType et, int errornum = 0);
	/** Dispatched from HandleEvent */
	virtual void DoRead();
	/** Dispatched from HandleEvent */
	virtual void DoWrite();

	/** Sets the error message for this socket. Once set, the socket is dead. */
	void SetError(const std::string& err) { if (error.empty()) error = err; }

	/** Gets the error message for this socket. */
	const std::string& getError() const { return error; }

	/** Called when new data is present in recvq */
	virtual void OnDataReady() = 0;
	/** Called when the socket gets an error from socket engine or IO hook */
	virtual void OnError(BufferedSocketError e) = 0;

	/** Send the given data out the socket, either now or when writes unblock
	 */
	void WriteData(const std::string& data);
	/** Convenience function: read a line from the socket
	 * @param line The line read
	 * @param delim The line delimiter
	 * @return true if a line was read
	 */
	bool GetNextLine(std::string& line, char delim = '\n');
	/** Useful for implementing sendq exceeded */
	inline size_t getSendQSize() const { return sendq_len; }

	/**
	 * Close the socket, remove from socket engine, etc
	 */
	virtual void Close();
	/** This ensures that close is called prior to destructor */
	virtual CullResult cull();
};
/**
 * BufferedSocket is an extendable socket class which modules
 * can use for TCP socket support. It is fully integrated
 * into InspIRCds socket loop and attaches its sockets to
 * the core's instance of the SocketEngine class, meaning
 * that all use is fully asynchronous.
 *
 * To use BufferedSocket, you must inherit a class from it.
 */
class CoreExport BufferedSocket : public StreamSocket
{
 public:
	/** Timeout object or NULL
	 */
	SocketTimeout* Timeout;

	/**
	 * The state for this socket, either
	 * listening, connecting, connected
	 * or error.
	 */
	BufferedSocketState state;

	BufferedSocket();
	/**
	 * This constructor is used to associate
	 * an existing connecting with an BufferedSocket
	 * class. The given file descriptor must be
	 * valid, and when initialized, the BufferedSocket
	 * will be placed in CONNECTED state.
	 */
	BufferedSocket(int newfd);

	/** Begin connection to the given address
	 * This will create a socket, register with socket engine, and start the asynchronous
	 * connection process. If an error is detected at this point (such as out of file descriptors),
	 * OnError will be called; otherwise, the state will become CONNECTING.
	 * @param dest Address to connect to
	 * @param bind Address to bind to (if NULL, no bind will be done)
	 * @param timeout Time to wait for connection
	 */
	void DoConnect(const std::string &ipaddr, int aport, unsigned long maxtime, const std::string &connectbindip);

	/** This method is called when an outbound connection on your socket is
	 * completed.
	 */
	virtual void OnConnected();

	/** When there is data waiting to be read on a socket, the OnDataReady()
	 * method is called.
	 */
	virtual void OnDataReady() = 0;

	/**
	 * When an outbound connection fails, and the attempt times out, you
	 * will receive this event.  The method will trigger once maxtime
	 * seconds are reached (as given in the constructor) just before the
	 * socket's descriptor is closed.  A failed DNS lookup may cause this
	 * event if the DNS server is not responding, as well as a failed
	 * connect() call, because DNS lookups are nonblocking as implemented by
	 * this class.
	 */
	virtual void OnTimeout();

	virtual ~BufferedSocket();
 protected:
	virtual void DoWrite();
	BufferedSocketError BeginConnect(const irc::sockets::sockaddrs& dest, const irc::sockets::sockaddrs& bind, unsigned long timeout);
	BufferedSocketError BeginConnect(const std::string &ipaddr, int aport, unsigned long maxtime, const std::string &connectbindip);
};

class CoreExport UserIOHandler : public StreamSocket
{
 public:
	LocalUser* const user;
	UserIOHandler(LocalUser* me) : user(me) {}
	void OnDataReady();
	void OnError(BufferedSocketError error);

	/** Adds to the user's write buffer.
	 * You may add any amount of text up to this users sendq value, if you exceed the
	 * sendq value, the user will be removed, and further buffer adds will be dropped.
	 * @param data The data to add to the write buffer
	 */
	void AddWriteBuf(const std::string &data);
};

#endif
