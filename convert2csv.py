#!/usr/bin/env python3
# -*- coding:utf-8 -*-
###
# File: /home/craft/workspace/memperf/convert2csv.py
# Project: /home/craft/workspace/memperf
# Created Date: Friday, April 21st 2023, 2:24:56 pm
# Author: zhangdanfeng
# -----
# Last Modified: Sun Apr 23 2023
# Modified By: zhangdanfeng
# -----
# Copyright (c) 2023 SpacemiT
# 
# All shall be well and all shall be well and all manner of things shall be well.
# Nope...we're doomed!
# -----
# -----------------------------------------------------------------------
###

import sys
import csv

def main():
    args = sys.argv[1:]
    filename = args[0]
    print("openning file " + filename + " ...")

    chart = []
    with open(filename,'r') as f:
        for line in f:
            chart.append(line.split())
            print(line.split())


    with open(filename + ".csv", 'w', newline='') as csvfile:
        csvwriter = csv.writer(csvfile)
        for row in chart:
            csvwriter.writerow(row)

if __name__ == '__main__':
    main()
