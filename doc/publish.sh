#!/bin/bash
rsync --delete -vaP sphinx/build/html/ ai.larroy.com:public_html/mycelium/sphinx/
rsync --delete -vaP doxygen/html/ ai.larroy.com:public_html/mycelium/doxygen/
