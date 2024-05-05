#! /bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
pushd $SCRIPT_DIR/../

if ./bin/con tests/step1/valid.json 2> /dev/null; then
	printf "success\n"
else
	printf "step 1 valid json test failed\n"
fi

if ! ./bin/con tests/step1/invalid.json 2> /dev/null; then
	printf "step 1 invalid json test failed\n"
else
	printf "success\n"
fi

popd
