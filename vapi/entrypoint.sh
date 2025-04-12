#!/bin/bash
docker build -t vapi .
python3 user_prompt.py
