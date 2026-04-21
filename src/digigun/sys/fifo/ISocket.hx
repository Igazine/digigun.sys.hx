package digigun.sys.fifo;

/**
 * Interface for Local Unix Socket specific operations.
 */
interface ISocket extends ICommunicator {
    /**
     * Binds the socket to a specific filesystem path.
     * @param path The filesystem path for the socket.
     * @return True if successful, false otherwise.
     */
    function bind(path:String):Bool;

    /**
     * Listens for incoming connections on the socket.
     * @param backlog The maximum number of pending connections.
     * @return True if successful, false otherwise.
     */
    function listen(backlog:Int = 5):Bool;

    /**
     * Accepts a new incoming connection.
     * @return A new ISocket instance for the accepted connection, or null on error.
     */
    function accept():ISocket;

    /**
     * Connects to a socket at the specified path.
     * @param path The filesystem path of the socket.
     * @return True if successful, false otherwise.
     */
    function connect(path:String):Bool;
}
