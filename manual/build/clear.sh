#!/bin/sh

basedir="$(cd "$(dirname $0)"; pwd)"
manual_path="$(echo $basedir | sed "s:/build$::")"
build_path="$manual_path/build"
cd $build_path

if [ -z "$destdir" ]; then
    destdir="build.out"
fi

if [ -z "$tempdir" ]; then
    tempdir="build.tmp"
fi

rm -rf "$destdir"
rm -rf "$tempdir"
