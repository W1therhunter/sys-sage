#include "defines.hpp"

// #ifdef PYBIND

#include <cstdio>
#include <exception>
#include <libxml2/libxml/parser.h>
#include <optional>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/attr.h>
#include <string>
#include <tuple>
#include "Component.hpp"
#include "DataPath.hpp"
#include "sys-sage.hpp"

namespace py = pybind11;

std::vector<std::string> default_attribs = {"CATcos","CATL3mask","mig_size","Number_of_streaming_multiprocessors","Number_of_cores_in_GPU","Number_of_cores_per_SM","Bus_Width_bit","Clock_Frequency","latency","latency_min","latency_max","CUDA_compute_capability","mig_uuid","freq_history","GPU_Clock_Rate"};

py::function print_attributes;
py::function print_complex_attributes;

py::function read_attributes;
py::function read_complex_attributes;

int xmldumper(std::string key, void* value, std::string* ret_value_str) {
    //
    if(default_attribs.end() != std::find(default_attribs.begin(), default_attribs.end(), key))
        return 0;
    auto * ptr = static_cast<std::shared_ptr<py::object>*>(value);
    py::object res = print_attributes( py::cast(key), *ptr->get());
    //check if res is none
    if(res.is_none())
        return 0;
    *ret_value_str = res.cast<std::string>();
    return 1;
}

int xmldumper_complex(std::string key, void* value, xmlNodePtr node) {
    if(default_attribs.end() != std::find(default_attribs.begin(), default_attribs.end(), key))
        return 0;
    auto * ptr = static_cast<std::shared_ptr<py::object>*>(value);
    //idea: we expect fcn to return xml as a string and write it to node
    py::object res = print_complex_attributes(py::cast(key), *ptr->get());
    //check if res is none
    if(res.is_none())
        return 0;
    //xmlBufferPtr buffer = xmlBufferCreate();
    std::string xml_str = res.cast<std::string>();
    xmlDocPtr doc = xmlParseDoc((const xmlChar*)xml_str.c_str());
    xmlNodePtr root = xmlDocGetRootElement(doc);
    xmlAddChild(node,root->children);
    return 0;
}

void* xmlloader(xmlNodePtr node) {
    //idea: we expect fcn to return xml as a string and write it to node
    xmlBufferPtr buffer = xmlBufferCreate();
    try{
        xmlNodeDump(buffer, node->doc, node, 0, 1);
        std::string xml_str((const char*)xmlBufferContent(buffer));
        xmlBufferFree(buffer);
        py::object value = read_attributes(py::cast(xml_str));
        //check for values content
        if (value.is_none()) {
            return NULL;
        }
        std::shared_ptr<py::object> *ptr = new std::shared_ptr<py::object>(std::make_shared<py::object>(value));
        return static_cast<void*>(ptr);
    }
    catch(std::exception &e){
        std::cout << e.what() << std::endl;
        return NULL;
    }
}

int xmlloader_complex(xmlNodePtr node, Component *c) {
    xmlBufferPtr buffer = xmlBufferCreate();
    try{
        xmlNodeDump(buffer, node->doc, node, 0, 1);
        std::string xml_str((const char*)xmlBufferContent(buffer));
        xmlBufferFree(buffer);
        py::object comp = py::cast(c);
        py::object value = read_complex_attributes(py::cast(xml_str),comp);
        if (value.is_none())
            return 0;
        return 1;
    }
    catch(std::exception &e){
        std::cout << e.what() << std::endl;
    }
    return 0;
}

//
void set_attribute(Component &self, const std::string &key, py::object &value) {
    if(default_attribs.end() != std::find(default_attribs.begin(), default_attribs.end(), key))
        throw py::type_error("Attribute " + key + " is read-only");
    auto obj = new std::shared_ptr<py::object>(std::make_shared<py::object>(value));
    // free existing value first
    auto val = self.attrib.find(key);
    if (val != self.attrib.end()) {
        delete static_cast<std::shared_ptr<py::object>*>(val->second);
    }
    self.attrib[key] = static_cast<void*>(obj);
}

py::object get_attribute(Component &self, const std::string &key) {
    auto val = self.attrib.find(key);
    if (val != self.attrib.end()) {
        if(!key.compare("CATcos") || !key.compare("CATL3mask")){
            uint64_t retval = *((uint64_t*)val->second); 
            return py::cast(retval);
        }
        else if(!key.compare("mig_size") )
        {
            return py::cast(*(long long*)val->second);
        }
        //val->secondue: int
        else if(!key.compare("Number_of_streaming_multiprocessors") || 
        !key.compare("Number_of_cores_in_GPU") || 
        !key.compare("Number_of_cores_per_SM")  || 
        !key.compare("Bus_Width_bit") )
        {
            return py::cast(*(int*)val->second);
        }
        //value: double
        else if(!key.compare("Clock_Frequency") )
        {
            return py::cast(*(double*)val->second);
        }
        //value: float
        else if(!key.compare("latency") ||
        !key.compare("latency_min") ||
        !key.compare("latency_max") )
        {
            return py::cast(*(float*)val->second);
        }   
        //value: string
        else if(!key.compare("CUDA_compute_capability") || 
        !key.compare("mig_uuid") )
        {
            return py::cast(*(string*)val->second);
        }
        else if(!key.compare("freq_history") ){
            std::vector<std::tuple<long long,double>>* value = (std::vector<std::tuple<long long,double>>*)(val->second);
            py::dict freq_dict;
             for(auto [ ts,freq ] : *value){
                 freq_dict[py::cast(ts)] = py::cast(freq);
                printf("ts:%lld freq:%f\n",ts,freq);
             }
             return freq_dict;
        }
        else if(!key.compare("GPU_Clock_Rate")){
            auto [ freq, unit ] = *(std::tuple<double, std::string>*)val->second;
            py::dict freq_dict;
            freq_dict[py::str("freq")] = py::cast(freq);
            freq_dict[py::str("unit")] = py::cast(unit);       
            return freq_dict;
        }else{
            auto * ptr = static_cast<std::shared_ptr<py::object>*>(val->second);
            return *ptr->get();
        }
    } else {
        throw py::attribute_error("Attribute '" + key + "' not found"); 
    }
}

void remove_attribute(Component &self, const std::string &key) {
    auto val = self.attrib.find(key);
    if (val != self.attrib.end()) {
        delete static_cast<std::shared_ptr<py::object>*>(val->second);
        self.attrib.erase(val);
    } else {
        throw py::attribute_error("Attribute " + key + " not found");
    }
}


PYBIND11_MODULE(sys_sage, m) {

        m.attr("COMPONENT_NONE") = SYS_SAGE_COMPONENT_NONE;
        m.attr("COMPONENT_THREAD") = SYS_SAGE_COMPONENT_THREAD;
        m.attr("COMPONENT_CORE") = SYS_SAGE_COMPONENT_CORE;
        m.attr("COMPONENT_CACHE") = SYS_SAGE_COMPONENT_CACHE;
        m.attr("COMPONENT_SUBDIVISION") = SYS_SAGE_COMPONENT_SUBDIVISION;
        m.attr("COMPONENT_NUMA") = SYS_SAGE_COMPONENT_NUMA;
        m.attr("COMPONENT_CHIP") = SYS_SAGE_COMPONENT_CHIP;
        m.attr("COMPONENT_MEMORY") = SYS_SAGE_COMPONENT_MEMORY;
        m.attr("COMPONENT_STORAGE") = SYS_SAGE_COMPONENT_STORAGE;
        m.attr("COMPONENT_NODE") = SYS_SAGE_COMPONENT_NODE;
        m.attr("COMPONENT_TOPOLOGY") = SYS_SAGE_COMPONENT_TOPOLOGY;

        m.attr("SUBDIVISION_TYPE_NONE") = SYS_SAGE_SUBDIVISION_TYPE_NONE;
        m.attr("SUBDIVISION_TYPE_GPU_SM") = SYS_SAGE_SUBDIVISION_TYPE_GPU_SM;

        m.attr("CHIP_TYPE_NONE") = SYS_SAGE_CHIP_TYPE_NONE;
        m.attr("CHIP_TYPE_CPU") = SYS_SAGE_CHIP_TYPE_CPU;
        m.attr("CHIP_TYPE_CPU_SOCKET") = SYS_SAGE_CHIP_TYPE_CPU_SOCKET;
        m.attr("CHIP_TYPE_GPU") = SYS_SAGE_CHIP_TYPE_GPU;

        m.attr("DATAPATH_NONE") = SYS_SAGE_DATAPATH_NONE;
        m.attr("DATAPATH_OUTGOING") = SYS_SAGE_DATAPATH_OUTGOING;
        m.attr("DATAPATH_INCOMING") = SYS_SAGE_DATAPATH_INCOMING;
        m.attr("DATAPATH_BIDIRECTIONAL") = SYS_SAGE_DATAPATH_BIDIRECTIONAL;
        m.attr("DATAPATH_ORIENTED") = SYS_SAGE_DATAPATH_ORIENTED;
        m.attr("DATAPATH_TYPE_NONE") = SYS_SAGE_DATAPATH_TYPE_NONE;
        m.attr("DATAPATH_TYPE_LOGICAL") = SYS_SAGE_DATAPATH_TYPE_LOGICAL;
        m.attr("DATAPATH_TYPE_PHYSICAL") = SYS_SAGE_DATAPATH_TYPE_PHYSICAL;
        m.attr("DATAPATH_TYPE_L3CAT") = SYS_SAGE_DATAPATH_TYPE_L3CAT;
        m.attr("DATAPATH_TYPE_MIG") = SYS_SAGE_DATAPATH_TYPE_MIG;
        m.attr("DATAPATH_TYPE_DATATRANSFER") = SYS_SAGE_DATAPATH_TYPE_DATATRANSFER;
        m.attr("DATAPATH_TYPE_C2C") = SYS_SAGE_DATAPATH_TYPE_C2C;

    //bind component class
    py::class_<Component>(m, "Component", /*py::dynamic_attr(),*/"Generic Component")
        .def(py::init<int, string, int>(), py::arg("id") = 0, py::arg("name") = "unknown", py::arg("type") = SYS_SAGE_COMPONENT_NONE)
        .def(py::init<Component *, int, string, int>(), py::arg("parent"), py::arg("id") = 0, py::arg("name") = "unknown", py::arg("type") = SYS_SAGE_COMPONENT_NONE)
        // .def("__setattr__", [](Component& self, const std::string& name, py::object value) {
        //     set_attribute(self,name, value);
        // })
        // .def("__getattr__", [](Component& self, const std::string& name) {
        //     return get_attribute(self,name);
        // })
        // .def("__delattr__", [](Component& self, const std::string& name) {
        //     remove_attribute(self,name);
        // })
        .def("InsertChild", &Component::InsertChild, py::arg("child"), "Insert a child component")
        .def("InsertBetweenParentAndChild", &Component::InsertBetweenParentAndChild, py::arg("parent"), py::arg("child"), py::arg("alreadyParentsChild"),"Insert a component between parent and child")
        .def("InsertBetweenParentAndChildren", &Component::InsertBetweenParentAndChildren, py::arg("parent"), py::arg("children"), py::arg("alreadyParentsChildren"), "Insert a component between parent and children")
        .def("RemoveChild", &Component::RemoveChild, py::arg("child"),"Remove a child component")
        .def_property("parent", &Component::GetParent, &Component::SetParent, "The parent of the component")
        .def("GetParent", &Component::GetParent, "The parent of the component")
        .def("SetParent", &Component::SetParent, py::arg("parent"), "The parent of the component")
        .def("PrintSubtree", (void (Component::*)()) &Component::PrintSubtree, "Print the subtree of the component up to level 0")
        .def("PrintSubtree", (void (Component::*)(int)) &Component::PrintSubtree, "Print the subtree of the component with a maximum depth of <level>")
        .def("PrintAllDataPathsInSubtree", &Component::PrintAllDataPathsInSubtree, "Print the datapath subtree of the component")
        .def_property("name", &Component::GetName, &Component::SetName, "The name of the component")
        .def_property_readonly("id", &Component::GetId, "The id of the component")
        .def_property_readonly("type", &Component::GetComponentType, "The type of the component")
        .def("GetComponentTypeStr", &Component::GetComponentTypeStr, "The type of the component as string")
        .def("GetChildren", &Component::GetChildren, "The children of the component")
        .def("GetChild", &Component::GetChild, py::arg("id"), "Like get_child_by_id()")
        .def("GetChildById", &Component::GetChildById, py::arg("id"), "Get the first child component by id")
        .def("GetChildByType", &Component::GetChildByType, py::arg("type"), "Get the first child component by type")
        .def("GetAllChildrenByType", (void (Component::*)(vector<Component*> *, int)) &Component::GetAllChildrenByType, py::arg("children"), py::arg("type"), "Get all child components by type")
        .def("GetAllChildrenByType", (vector<Component*> (Component::*)(int))(&Component::GetAllChildrenByType), py::arg("type"), "Get all child components by type")
        .def("GetSubcomponentById", &Component::GetSubcomponentById, "Get the first sub component by id")
        .def("GetAllSubcomponentsByType", (void (Component::*)(vector<Component*> *, int))(&Component::GetAllSubcomponentsByType), py::arg("subcomponents"), py::arg("type"), "Get the first sub component by type")
        .def("GetAllSubcomponentsByType", (vector<Component*> (Component::*)(int))(&Component::GetAllSubcomponentsByType),py::arg("type") ,"Get all sub components by type")
        .def("CountAllSubcomponents", &Component::CountAllSubcomponents, "Count all sub components")
        .def("CountAllSubcomponentsByType", &Component::CountAllSubcomponentsByType, py::arg("type"),"Count sub components by type")
        .def("CountChildrenByType", &Component::CountAllChildrenByType,py::arg("type"),"Count children by type")
        .def("GetAncestorByType", &Component::GetAncestorByType, py::arg("type"),"Get the first ancestor component by type")
        .def("GetSubtreeDepth", &Component::GetSubtreeDepth, "Get the depth of the subtree")
        .def("GetNthAncestor", &Component::GetNthAncestor, py::arg("n"),"Get the nth ancestor of the component")
        .def("GetNthDescendents", (void (Component::*)(vector<Component*> *, int))&Component::GetNthDescendents,py::arg("descendents"), py::arg("n"),"Get all the nth descendents of the component")
        .def("GetNthDescendents", (vector<Component*> (Component::*)(int))&Component::GetNthDescendents,py::arg("n"),"Get all the nth descendents of the component")
        .def("GetSubcomponentsByType", (void (Component::*)(vector<Component*> *, int))&Component::GetSubcomponentsByType,py::arg("subcomponents"), py::arg("type"),"Get all the sub components of the component by type")
        .def("GetSubcomponentsByType", (vector<Component*> (Component::*)(int))&Component::GetSubcomponentsByType,py::arg("type"),"Get all the sub components of the component by type")
        .def("GetComponentsInSubtree", (void (Component::*)(vector<Component*> *))&Component::GetComponentsInSubtree,py::arg("components"),"Get all the components in the subtree of the component")
        .def("GetComponentsInSubtree", (vector<Component*> (Component::*)())&Component::GetComponentsInSubtree,"Get all the components in the subtree of the component")
        .def("GetSubcomponentById", &Component::GetSubcomponentById, py::arg("id"),py::arg("type"),"Get the first sub component by id and type")
        .def("GetDataPaths", &Component::GetDataPaths,py::arg("orientation"),"Get all the data paths associated with the component")
        .def("AddDataPath", &Component::AddDataPath,py::arg("datapath"), py::arg("orientation"),"Add a data path to the component")
        .def("GetDataPathByType", &Component::GetDataPathByType, py::arg("type"), py::arg("orientation"),"Get the first data path associated with the component by type")
        .def("GetAllDataPathsByType", (void (Component::*)(vector<DataPath*> *, int, int))&Component::GetAllDataPathsByType, py::arg("datapaths"), py::arg("type"), py::arg("orientation"),"Get all the data paths associated with the component by type")
        .def("GetAllDataPathsByType", (vector<DataPath*> (Component::*)(int, int)) &Component::GetAllDataPathsByType,py::arg("type"), py::arg("orientation"),"Get all the data paths associated with the component by type")
        .def("CheckComponentTreeConsistency", &Component::CheckComponentTreeConsistency,"Check if the component tree is consistent")
        .def("GetTopologySize", (int (Component::*)(unsigned*, unsigned*)) (&Component::GetTopologySize),py::arg("components_size"),py::arg("datapaths_size"),"Get the size of the topology")
        .def("GetTopologySize", (int (Component::*)(unsigned*, unsigned*, std::set<DataPath*>*)) (&Component::GetTopologySize),py::arg("components_size"),py::arg("datapaths_size"),py::arg("datapaths_counted"),"Get the size of the topology")
        .def("GetDepth", &Component::GetDepth,py::arg("refresh"),"Get the depth of the component, if refresh is true it will update the depth")
        .def("DeleteDataPath", &Component::DeleteDataPath, py::arg("datapath"),"Delete a data path from the component")
        .def("DeleteAllDataPaths", &Component::DeleteAllDataPaths,"Delete all the data paths from the component")
        .def("DeleteSubtree", &Component::DeleteSubtree,"Delete the subtree of the component")
        .def("Delete", &Component::Delete,py::arg("withSubtree") = true,"Delete the component")
        .def("__bool__",[](Component& self){
            std::vector<Component*>* children = self.GetChildren();
            return !children->empty();
        })
        .def("__repr__", [](Component& self) {
            //TODO: add more info
            return "<Component: " + self.GetName() + ">";
            });
    py::class_<Topology,Component>(m, "Topology")
        .def(py::init<>());
    py::class_<Node, Component>(m, "Node")
        .def(py::init<int, string>(), py::arg("id") = 0, py::arg("name")= "Node")
        .def(py::init<Component*, int, string>(), py::arg("parent"), py::arg("id") = 0, py::arg("name") = "Node")
        .def("RefreshCpuCoreFrequency", &Node::RefreshCpuCoreFrequency, py::arg("keep_history")=false,"Refresh the cpu core frequency");
    py::class_<Memory, Component>(m, "Memory")
        .def(py::init<long long, bool>(), py::arg("size") = -1, py::arg("isVolatile") = false)
        .def(py::init<Component*,int, string, long long, bool>(), py::arg("parent"), py::arg("id") = 0, py::arg("name") = "Memory", py::arg("size")=-1, py::arg("isVolatile")=false)
        .def_property("size", &Memory::GetSize, &Memory::SetSize, "The size of the memory")
        .def_property("isVolatile", &Memory::GetIsVolatile, &Memory::SetIsVolatile, "Whether the memory is volatile or not");
    py::class_<Storage, Component>(m, "Storage")
        .def(py::init<long long>(), py::arg("size")=-1)
        .def(py::init<Component*,long long>(), py::arg("parent"), py::arg("size")= -1)
        .def_property("size", &Storage::GetSize, &Storage::SetSize, "The size of the storage");
    py::class_<Chip, Component>(m, "Chip")
        .def(py::init<int,string,int,string,string>(), py::arg("id") = 0, py::arg("name") = "Chip", py::arg("chipType")= 1, py::arg("vendor") = "", py::arg("model") = "", py::return_value_policy::reference)
        .def(py::init<Component*,int,string,int,string,string>(), py::arg("parent"),py::arg("id") = 0, py::arg("name") = "Chip", py::arg("chipType") = 1, py::arg("vendor") = "", py::arg("model") = "")
        .def_property("vendor", &Chip::GetVendor, &Chip::SetVendor, "The vendor of the chip")
        .def_property("model", &Chip::GetModel, &Chip::SetModel, "The model of the chip")
        .def_property("chipType", &Chip::GetChipType, &Chip::SetChipType, "The type of the chip");
    py::class_<Cache, Component>(m, "Cache")
        .def(py::init<int,int,long long, int, int>(), py::arg("id") = 0, py::arg("level") = 0, py::arg("size") = -1, py::arg("associativity") = -1, py::arg("lineSize") = -1)
        .def(py::init<Component*,int,int,long long, int, int>(), py::arg("parent"), py::arg("id") = 0, py::arg("level") = 0, py::arg("size") = -1, py::arg("associativity") = -1, py::arg("lineSize") = -1)
        .def(py::init<Component*, int, string, long long, int, int>(), py::arg("parent"), py::arg("id"), py::arg("cache_type"), py::arg("size") = -1, py::arg("associativity") = -1, py::arg("lineSize") = -1)
        .def_property("cacheLevel", &Cache::GetCacheLevel, &Cache::SetCacheLevel, "The level of the cache")
        .def_property("cacheName", &Cache::GetCacheName, &Cache::SetCacheName, "The name of the cache")
        .def_property("cacheSize", &Cache::GetCacheSize, &Cache::SetCacheSize, "The size of the cache")
        .def_property("cacheAssociativity", &Cache::GetCacheAssociativityWays, &Cache::SetCacheAssociativityWays, "The associativity of the cache")
        .def_property("cacheLineSize", &Cache::GetCacheLineSize, &Cache::SetCacheLineSize, "The line size of the cache");
    py::class_<Subdivision, Component>(m, "Subdivision")
        .def(py::init<int,string, int>(), py::arg("id") = 0, py::arg("name") = "Subdivision", py::arg("componentType") = 16)
        .def(py::init<Component*,int,string, int>(), py::arg("parent"), py::arg("id") = 0, py::arg("name") = "Subdivision", py::arg("componentType") = 16)
        .def_property("subdivisionType", &Subdivision::GetSubdivisionType, &Subdivision::SetSubdivisionType, "The type of the subdivision");
    py::class_<Numa, Subdivision>(m, "Numa")
        .def(py::init<int, long long>(), py::arg("id") = 0, py::arg("size") = -1)
        .def(py::init<Component*, int, long long>(), py::arg("parent"), py::arg("id") = 0, py::arg("size") = -1)
        .def_property("size", &Numa::GetSize, &Numa::SetSize, "Size of the NUMA region");
    py::class_<Core, Component>(m, "Core")
        .def(py::init<int,string>(),py::arg("id") = 0, py::arg("name") = "Core")
        .def(py::init<Component*,int,string>(),py::arg("parent"),py::arg("id") = 0 ,py::arg("name") = "Core")
        .def("RefreshFreq", &Core::RefreshFreq,py::arg("keep_history") = false,"Refresh the frequency of the component")
        .def_property("freq", &Core::GetFreq, &Core::SetFreq, "Frequency of this core");
    py::class_<Thread,Component>(m,"Thread")
        .def(py::init<int,string>(),py::arg("id") = 0,py::arg("name") = "Thread")
        .def(py::init<Component*,int,string>(),py::arg("parent"),py::arg("id") = 0,py::arg("name") = "Thread")
        .def("RefreshFreq", &Thread::RefreshFreq,py::arg("keep_history") = false,"Refresh the frequency of the component")
        .def_property_readonly("freq", &Thread::GetFreq, "Get Frequency of this thread");
    py::class_<DataPath>(m,"DataPath",py::dynamic_attr())
        .def(py::init<Component*, Component*, int, int>(), py::arg("source"), py::arg("target"), py::arg("oriented"), py::arg("type") = 32)
        .def(py::init<Component*, Component*, int, double, double>(), py::arg("source"), py::arg("target"), py::arg("oriented"), py::arg("bw"), py::arg("latency"))
        .def(py::init<Component*, Component*, int, int, double, double>(), py::arg("source"), py::arg("target"), py::arg("oriented"), py::arg("type"), py::arg("bw"), py::arg("latency"))
        .def_property("bandwidth", &DataPath::GetBandwidth, &DataPath::SetBandwidth, "The bandwidth of the data path")
        .def_property("latency", &DataPath::GetLatency, &DataPath::SetLatency, "The latency of the data path")
        .def_property_readonly("type", &DataPath::GetDataPathType, "The type of the data path")
        .def_property_readonly("oriented", &DataPath::GetOrientation, "The orientation of the data path")
        .def_property("source", &DataPath::GetSource, &DataPath::UpdateSource, "The source of the data path")
        .def_property("target", &DataPath::GetTarget, &DataPath::UpdateTarget, "The target of the data path");

    m.def("parseMt4gTopo", (int (*) (Node*,string,int, string)) &parseMt4gTopo, "parseMt4gTopo", py::arg("parent"), py::arg("dataSourcePath"), py::arg("gpuID"), py::arg("delim") = ";");
    m.def("parseMt4gTopo", (int (*) (Component*,string,int, string)) &parseMt4gTopo, "parseMt4gTopo", py::arg("parent"), py::arg("dataSourcePath"), py::arg("gpuID"), py::arg("delim") = ";");
    m.def("parseMt4gTopo", (int (*) (Chip*,string, string)) &parseMt4gTopo, "parseMt4gTopo", py::arg("parent"), py::arg("dataSourcePath"),  py::arg("delim") = ";");

    m.def("parseHwlocOutput", &parseHwlocOutput, "parseHwlocOutput", py::arg("root"), py::arg("xmlPath"));

    m.def("parseCccbenchOutput", &parseCccbenchOutput, "parseCccbenchOutput", py::arg("root"), py::arg("cccPath"));

    m.def("parseCapsNumaBenchmark", &parseCapsNumaBenchmark,  py::arg("root"), py::arg("benchmarkPath"), py::arg("delim") = ";");

    m.def("exportToXml", [](Component& root, string xmlPath, std::optional<py::function> print_att = std::nullopt, std::optional<py::function> print_catt = std::nullopt) {
        if(print_att)
            print_attributes = *print_att;
        if(print_catt)
            print_complex_attributes = *print_catt;
        exportToXml(&root, xmlPath,print_att ? xmldumper : nullptr,print_catt ? xmldumper_complex : nullptr);
    },py::arg("root"), py::arg("xmlPath") = "out.xml", py::arg("print_att") = py::none(), py::arg("print_catt") = py::none());
    
    m.def("importFromXml",[](string path, std::optional<py::function> search_custom_attrib_key_fcn = std::nullopt, std::optional<py::function> search_custom_complex_attrib_key_fcn = std::nullopt) {
        if(search_custom_attrib_key_fcn)
            read_attributes = *search_custom_attrib_key_fcn;
        if(search_custom_complex_attrib_key_fcn)
            read_complex_attributes = *search_custom_complex_attrib_key_fcn;
        return importFromXml(path,search_custom_attrib_key_fcn ? xmlloader : nullptr, search_custom_complex_attrib_key_fcn ? xmlloader_complex : nullptr );
    }, py::arg("path"), py::arg("search_custom_attrib_key_fcn") = py::none(), py::arg("search_custom_complex_attrib_key_fcn") = py::none());
}


// #endif