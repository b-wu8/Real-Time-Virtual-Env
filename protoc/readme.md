# How to use protobuf on our client
* Use the nuget manager in Visual Studio to download Google Protobuf
* Then find the installed 4 precompiled runtime library files:
  1. packages\Google.Protobuf.3.17.0\lib\net45\Google.Protobuf.dll
  2. packages\System.Buffers.4.4.0\lib\netstandard2.0\System.Buffers.dll
  3. packages\System.Memory.4.5.3\lib\netstandard2.0\System.Memory.dll
  4. packages\System.Runtime.CompilerServices.Unsafe.4.5.2\lib\netstandard2.0\ System.Runtime.CompilerServices.Unsafe.dll 

## Method 3
* Compile these library yourself from protobuf git repo using the protobuf.sln(visual studio solution file)

# How to use protobuf on our server
## Compile
* Follow the below protobuf-c steps to install proto on server machine
## Change make file
* Add these two flag in the makefile:
  1. CFLAGS	   += `pkg-config --cflags 'libprotobuf-c >= 1.0.0'`
  2. LDFLAGS	   += `pkg-config --libs 'libprotobuf-c >= 1.0.0'` 
* compile with the generated c file(game.pb-c.c for example)

# How to write code for C# and C
## Method 1
* Follow the below reference link
## Method 2
* see changes in this 2 commits
  * [for oculus_server.c and unity/OculusClient.cs](https://github.com/melodymhsu/cs717-oculus/commit/a60eb3f5e9214e383ab6530796fe1bdc36a91f54#diff-f434a0b9484d43f68e4a5032de346075cbc571a0463641425f115c2079c77794)
  * [for oculus_server.c](https://github.com/melodymhsu/cs717-oculus/commit/f010d2f7d0b955049476a5ddaeb4db8e381d4b11)


# Code generation from .proto file
## For C code
protoc -I=. --c_out=../server_programs/protoc_generated ./game.proto

## For C# code
protoc -I=. --csharp_out=../unity/protoc_generated ./game.proto

# References
* [Proto Sytax(proto3)](https://developers.google.com/protocol-buffers/docs/proto3)
* [C# Usage](https://developers.google.com/protocol-buffers/docs/csharptutorial)
* [Protobuf-c wiki](https://github.com/protobuf-c/protobuf-c/wiki)