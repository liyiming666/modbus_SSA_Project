<link rel="stylesheet" href="http://yandex.st/highlightjs/6.1/styles/vs.css">  
<script src="http://yandex.st/highlightjs/6.1/highlight.min.js"></script>  
<script>  
hljs.tabReplace = ' ';  
hljs.initHighlightingOnLoad();  
</script>  

# SSA
ServerSide Agent

## Introduction  
* Overview:  
This is a tcp-based application layer protocol, which is user-defined. The client contains a registration package to identify the distinguishing device. The packet contains three parts: the header (including the identifier, version number, and uplink and downlink data identifier), the data part, and the crc16 check digit.

## Directorys  
* bin:  Binary executable file  
* build:  Target file  
* conf:  Configuration files  
* include:  Header files  
* src:  Source files  
* test:  Unit test files  
* Makefile:  Compile script  
* run.sh:  Execution script

## Prerequisites  
* c++11
* linux >= 2.6.32
* libcurl >= 7.26.0
* libboost >= 1.58.0
* gcc >= 4.7

## Usage:
* Download the automated deployment script:  
```  
        git clone http://219.143.218.236:8082/build.sh/build.sh.git  
```  
* Execute:  This script will automatically pull and compile the SDK and SSA source code.
```  
        ./build.sh clean && ./build.sh
```  
* Configure your credential:  
    Credential file in the BQAQ/conf directory.Modify the contents as the following:  
```  
        <tenant>
        <username>
        <password>
    such as:
        test
        test_device
        hello@123
```  
* Execution:  
```  
        cd BQAQ
        ./bin/run.sh
```

## Configuration file description:

### You can manually configure the following items
* server:  Which tenant (domain or ip address) you want to connect to?  
* device_id:  Root device unique identifier.
* credential_path:  credential file.
* timer:  Set timer, client heartbeat timeout.
* template_dir:  Smartrest1.0 file.
* port:  SSA listening tcp port.
* interval:  Root device online time.

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
