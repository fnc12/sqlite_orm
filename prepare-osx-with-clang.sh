#!/bin/sh

# exit on firts error
set -e

brew install llvm
brew info llvm
export PATH="/usr/local/opt/llvm/bin:$PATH"
echo 'export PATH="/usr/local/opt/llvm/bin:$PATH"' >> ~/.bash_profile
