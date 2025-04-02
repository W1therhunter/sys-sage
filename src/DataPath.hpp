#ifndef DATAPATH
#define DATAPATH

#include <map>

#include "defines.hpp"
#include "Component.hpp"

//Component pointing to a DataPath 
#define SYS_SAGE_DATAPATH_NONE 1 /**< Reserved for development purposes. */
#define SYS_SAGE_DATAPATH_OUTGOING 2 /**< This Component is the source DataPath. */
#define SYS_SAGE_DATAPATH_INCOMING 4 /**< This Component is the taerget DataPath. */

//int oriented
#define SYS_SAGE_DATAPATH_BIDIRECTIONAL 8 /**< DataPath has no direction. */
#define SYS_SAGE_DATAPATH_ORIENTED 16 /**< DataPath is directec from the source to the target. */

//dp_type
#define SYS_SAGE_DATAPATH_TYPE_NONE 32 /**< Generic type of DataPath */
#define SYS_SAGE_DATAPATH_TYPE_LOGICAL 64 /**< DataPath describes a logical connection/relation of two Components. */
#define SYS_SAGE_DATAPATH_TYPE_PHYSICAL 128 /**< DataPath describes a physical/hardware connection/relation of two Components. */
#define SYS_SAGE_DATAPATH_TYPE_L3CAT 256 /**< DataPath type describing Cache partitioning settings. */
#define SYS_SAGE_DATAPATH_TYPE_MIG 512 /**< DataPath type describing GPU partitioning settings. */
#define SYS_SAGE_DATAPATH_TYPE_DATATRANSFER 1024 /**< DataPath type describing data transfer attributes. */
#define SYS_SAGE_DATAPATH_TYPE_C2C 2048 /**< DataPath type describing cache-to-cache latencies (cccbench data source). */

using namespace std;
class Component;
class DataPath;

/**
 @private
Obsolete; use DataPath() constructors directly instead
@see DataPath()
*/
[[deprecated("Use DataPath() constructors directly instead.")]]
DataPath* NewDataPath(Component* _source, Component* _target, int _oriented, int _type = SYS_SAGE_DATAPATH_TYPE_NONE);
/**
 * @private
Obsolete; use DataPath() constructors directly instead
@see DataPath()
*/
[[deprecated("Use DataPath() constructors directly instead.")]]
DataPath* NewDataPath(Component* _source, Component* _target, int _oriented, double _bw, double _latency);
/**
 * @private
Obsolete; use DataPath() constructors directly instead
@see DataPath()
*/
[[deprecated("Use DataPath() constructors directly instead.")]]
DataPath* NewDataPath(Component* _source, Component* _target, int _oriented, int _type, double _bw, double _latency);

/**
Class DataPath represents Data Paths in the topology -- Data Paths represent an arbitrary relation (or data movement) between two Components from the Component Tree.
\n Data Paths create a Data-Path graph, which is a structure orthogonal to the Component Tree.
\n Each Component contains a reference to all Data Paths going to or from this components (as well as parents and children in the Component Tree). Using these references, it is possible to navigate between the Components and to view the data stores in the Components or the Data Paths.
*/
class DataPath {

public:
    /**
    DataPath constructor.
    @param _source - pointer to the source Component. (If _oriented == SYS_SAGE_DATAPATH_BIDIRECTIONAL, there is no difference between _source and _target)
    @param _target - pointer to the target Component.
    @param _oriented - is the Data Path oriented? (SYS_SAGE_DATAPATH_ORIENTED = oriented DP; SYS_SAGE_DATAPATH_BIDIRECTIONAL = NOT oriented DP)
    @param _type - (optional parameter) Denotes type of the Data Path -- helps to distinguish Data Paths carrying different type of information. The value is user-defined. If not specified, the Data Path will have type SYS_SAGE_DATAPATH_TYPE_NONE.
        \n Predefined types: SYS_SAGE_DATAPATH_TYPE_NONE, SYS_SAGE_DATAPATH_TYPE_LOGICAL, SYS_SAGE_DATAPATH_TYPE_PHYSICAL, SYS_SAGE_DATAPATH_TYPE_L3CAT. Each user can define an arbitrary type as an integer value > 1024
    */
    DataPath(Component* _source, Component* _target, int _oriented, int _type = SYS_SAGE_DATAPATH_TYPE_NONE);
    /**
    DataPath constructor. DataPath type is set to SYS_SAGE_DATAPATH_TYPE_NONE.
    @param _source - pointer to the source Component. (If _oriented == SYS_SAGE_DATAPATH_BIDIRECTIONAL, there is no difference between _source and _target)
    @param _target - pointer to the target Component.
    @param _oriented - is the Data Path oriented? (SYS_SAGE_DATAPATH_ORIENTED = oriented DP; SYS_SAGE_DATAPATH_BIDIRECTIONAL = NOT oriented DP)
    @param _bw - bandwidth from the source(provides the data) to the target(requests the data)
    @param _latency - Data load latency from the source(provides the data) to the target(requests the data)
    */
    DataPath(Component* _source, Component* _target, int _oriented, double _bw, double _latency);
    /**
    DataPath constructor.
    @param _source - pointer to the source Component. (If _oriented == SYS_SAGE_DATAPATH_BIDIRECTIONAL, there is no difference between _source and _target)
    @param _target - pointer to the target Component.
    @param _oriented - is the Data Path oriented? (SYS_SAGE_DATAPATH_ORIENTED = oriented DP; SYS_SAGE_DATAPATH_BIDIRECTIONAL = NOT oriented DP)
    @param _type - (optional parameter) Denotes type of the Data Path -- helps to distinguish Data Paths carrying different type of information. The value is user-defined. If not specified, the Data Path will have type SYS_SAGE_DATAPATH_TYPE_NONE.
        \n Predefined types: SYS_SAGE_DATAPATH_TYPE_NONE, SYS_SAGE_DATAPATH_TYPE_LOGICAL, SYS_SAGE_DATAPATH_TYPE_PHYSICAL, SYS_SAGE_DATAPATH_TYPE_L3CAT. Each user can define an arbitrary type as an integer value > 1024
    @param _bw - Bandwidth from the source(provides the data) to the target(requests the data)
    @param _latency - Data load latency from the source(provides the data) to the target(requests the data)
    */
    DataPath(Component* _source, Component* _target, int _oriented, int _type, double _bw, double _latency);

    ~DataPath();

    /**
    @returns Pointer to the source Component
    */
    Component* GetSource();
    /**
    @returns Pointer to the target Component
    */
    Component* GetTarget();
    /**
    @returns Bandwidth from the source(provides the data) to the target(requests the data)
    */
    double GetBandwidth();
    
    /**
     * Sets the bandwidth from the source to the target.
    @param _bandwidth - value for bw
    @see bw
    */
    void SetBandwidth(double _bandwidth);
    /**
    @returns Data load latency from the source(provides the data) to the target(requests the data)
    */
    double GetLatency();

    /**
     * Sets the data load latency from the source to the target.
    @param _latency - value for latency
    @see latency
    */
    void SetLatency(double _latency);
    
    /**
     * Updates the source of the DataPath. The datapath is removed from the outgoing/incoming vectors of the old source and 
     * added to the vectors of the newer source.
     * @param _new_source - New source of the DataPath.
     */
    void UpdateSource(Component * _new_source);

    /**
     * Updates the target of the DataPath. The datapath is removed from the outgoing/incoming vectors of the old target and 
     * added to the vectors of the newer target.
     * @param _new_source - New target of the DataPath.
     */
    void UpdateTarget(Component * _new_target);
    
    /**
    @returns Type of the Data Path.
    @see dp_type
    */
    int GetDataPathType();
    /**
     * Retrieves the orientation of the datapath (SYS_SAGE_DATAPATH_ORIENTED or SYS_SAGE_DATAPATH_BIDIRECTIONAL)
     * @returns orientation 
     * @see oriented
     * 
    */
    int GetOrientation();

    /**
    Prints basic information about the Data Path to stdout. Prints componentType and Id of the source and target Components, the bandwidth, load latency, and the attributes; for each attribute, the name and value are printed, however the value is only retyped to uint64_t (therefore will print nonsensical values for other data types).
    */
    void Print();

    /**
    Deletes and de-allocates the DataPath pointer from the list(std::vector) of outgoing and incoming DataPaths of source and target Components.
    @see dp_incoming
    @see dp_outgoing
    */
    void DeleteDataPath();
    
    /**
    * A map for storing arbitrary pieces of information or data.
    * - The `key` denotes the name of the attribute.
    * - The `value` points to the data, stored as a `void*`.
    *
    * This data structure is designed to store a wide variety of data types by
    * utilizing pointers to void. Due to its flexibility, it is essential to manage
    * the types and memory allocation/deallocation carefully to avoid issues such
    * as memory leaks or undefined behavior.
    *
    * Usage:
    * 
    * 1. Adding a new key-value pair:
    * 
    * ```cpp
    * std::string key = "exampleKey";
    * int* value = new int(42); // Dynamically allocate memory for the value
    * attrib[key] = static_cast<void*>(value); // Store the value in the map
    * ```
    * 
    * 2. Retrieving data from an existing key:
    * 
    * ```cpp
    * std::string key = "exampleKey";
    * if (attrib.find(key) != attrib.end()) {
    *     int* retrievedValue = static_cast<int*>(attrib[key]);
    *     std::cout << "Value: " << *retrievedValue << std::endl;
    * } else {
    *     std::cout << "Key not found." << std::endl;
    * }
    * ```
    * 
    * 3. Checking for the existence of a key:
    * 
    * ```cpp
    * std::string key = "exampleKey";
    * if (attrib.find(key) != attrib.end()) {
    *     std::cout << "Key exists." << std::endl;
    * } else {
    *     std::cout << "Key does not exist." << std::endl;
    * }
    * ```
    * 
    * 4. Removing a key-value pair and freeing memory:
    * 
    * ```cpp
    * std::string key = "exampleKey";
    * if (attrib.find(key) != attrib.end()) {
    *     int* value = static_cast<int*>(attrib[key]);
    *     delete value; // Free the dynamically allocated memory
    *     attrib.erase(key); // Remove the key-value pair from the map
    * }
    * ```
    * 
    * 5. Updating the value for an existing key:
    * 
    * ```cpp
    * std::string key = "exampleKey";
    * if (attrib.find(key) != attrib.end()) {
    *     int* oldValue = static_cast<int*>(attrib[key]);
    *     delete oldValue; // Free the old value
    *     int* newValue = new int(100); // Allocate new value
    *     attrib[key] = static_cast<void*>(newValue); // Update the map
    * }
    * ```
    * 
    * Note:
    * - Proper memory management is crucial when using `void*` pointers. Always ensure
    *   that dynamically allocated memory is freed when no longer needed.
    * - Type safety is not enforced, so it is important to cast pointers to the correct
    *   type when retrieving values from the map.
    */
    std::map<std::string, void*> attrib;

private:
    Component * source; /**< source component of the datapath */
    Component * target; /**< taget component of the datapath */

    const int oriented; /**< orientation of the datapath (SYS_SAGE_DATAPATH_ORIENTED or SYS_SAGE_DATAPATH_BIDIRECTIONAL) */
    const int dp_type; /**< type of the datapath */

    double bw; /**< Bandwidth from the source(provides the data) to the target(requests the data) */
    double latency; /**< Data load latency from the source(provides the data) to the target(requests the data) */

};

#endif
