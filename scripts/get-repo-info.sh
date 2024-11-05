#!/bin/sh
# SPDX-License-Identifier: Apache-2.0

ACTION=$1
DEFAULT_URL="https://github.com/nubificus/vaccel"
DEFAULT_BRANCH="main"
DEFAULT_RAW_URL="https://raw.githubusercontent.com/nubificus/vaccel/main"

get_url() {
	url=$(git config --get remote.origin.url 2>/dev/null ||
		echo "${DEFAULT_URL}")
	echo "$url" | sed -e 's/^git:\/\//https:\/\//' \
		-e 's#^git@github.com:#https://github.com/#'
}

get_branch() {
	git rev-parse --abbrev-ref HEAD 2>/dev/null || echo "${DEFAULT_BRANCH}"
}

get_raw_url() {
	url=$(get_url)
	branch=$(get_branch)

	if echo "${url}" | grep -q 'github.com'; then
		owner_repo=$(echo "${url}" |
			sed -E -e 's#https://github.com/([^/]+/[^/]+)(\.git)?#\1#' \
				-e 's/\.git$//')
		echo "https://raw.githubusercontent.com/${owner_repo}/${branch}"
	else
		echo "${DEFAULT_RAW_URL}"
	fi
}

main() {
	case "${ACTION}" in
	url)
		get_url
		;;
	branch)
		get_branch
		;;
	rawurl)
		get_raw_url
		;;
	*)
		echo "Usage: $0 {url|branch|rawurl}"
		exit 1
		;;
	esac
}

main
