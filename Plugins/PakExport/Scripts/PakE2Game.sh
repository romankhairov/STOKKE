#!/bin/bash

Dir=$1

echo Dir=$Dir

#fix root path
for f in $(find "$Dir" -name '*.uasset' -or -name '*.umap'); do perl -pi -e 's/\/PakE./\/Game\//s' $f; done
