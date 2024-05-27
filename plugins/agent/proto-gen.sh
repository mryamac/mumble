#! /usr/bin/bash
protoc --proto_path=./ --cpp_out=./ ./mumble.proto