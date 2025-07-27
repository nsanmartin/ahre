#!/bin/bash
grep --include='*.[hc]'  -rhIo '"\(\\"\|[^"]\)*"' $1 | sort -u

