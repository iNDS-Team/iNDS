#!/bin/bash

if ! which jekyll &> /dev/null; then
    echo "jekyll is not installed. Install with gem install bundler jekyll."
    exit 1
fi

if ! which bundle &> /dev/null; then
    echo "bundle is not installed. Install with gem install bundler jekyll."
    exit 1
fi

if [[ -z "$JEKYLL_GITHUB_TOKEN" ]]; then
    echo "Env variable JEKYLL_GITHUB_TOKEN not found. Please create a Github \
access token with public_repo permissions."
    exit 1
fi

git pull
bundle install
bundle exec jekyll build