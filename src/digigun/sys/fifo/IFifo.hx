package digigun.sys.fifo;

/**
 * Interface for FIFO (Named Pipe) specific operations.
 */
interface IFifo extends ICommunicator {
    /**
     * Creates a FIFO at the specified path with the given mode (permissions).
     * @param path The filesystem path for the FIFO.
     * @param mode The creation mode (default is 0666).
     * @return True if successful, false otherwise.
     */
    function create(path:String, mode:Int = 438):Bool;

    /**
     * Opens an existing FIFO at the specified path.
     * @param path The filesystem path of the FIFO.
     * @param writeMode True if opening for writing, false for reading.
     * @return True if successful, false otherwise.
     */
    function open(path:String, writeMode:Bool):Bool;
}
