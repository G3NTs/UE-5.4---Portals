# Unreal Engine 5.4.2 Portals Demo Game

This repository contains the source code for a Portals Demo Game created in Unreal Engine 5.4.2 for CaptainCoder's Mechanically Challenged Game Jam.

![Demo GIF](https://github.com/G3NTs/UE-5.4---Portals/raw/main/UE5/Cpp/G3NTs/portal.gif)

## Overview

The game primarily utilizes C++ for the portal mechanics, with a small amount of C# and Blueprints where necessary. Each C++ function is documented using unreals documentation standard in both the `.hpp` and `.cpp` files. If you encounter any functions you don't understand, you can use Visual Studio's "Go to Definition" feature to view the source functions.

This project assumes you have a decent level of experience with Unreal Engine and C++. It is designed to help you implement more complex features that work in cohesion with the portal teleportation mechanic, a common point where many tutorials conclude.

## Features

- **Portal Scene Capture to Texture Target**
- **Scene Capture Clip Plane**: Prevents the virtual camera from being obscured by walls.
- **Player/Object Teleportation**
- **Player/Object Clip Plane**: Prevents objects from sticking through portals.
- **Player/Object Cloning**: See yourself come out through the other side.
- **Player Control Rotation Fixes**: Addresses a common bug where control rotation and actor rotation needed to be unlinked.
- **Player Character Animation Syncing**
- **Portal Edge Colliders**
- **Portal Edge Color Options**
- **Portal Gun**
- **Portal Wall Placement with Edge Detection**
- **Portal Placement Rotations**
- **Portal Overlap Prevention**
- **Portal Surface Dynamic Collision Meshes**

## Usage

To use this Unreal Engine repository, follow these steps:

1. Clone the repository to your local machine.
2. Open the project in Unreal Engine 5.4.2.
3. Build the project to compile the C++ code.
4. Explore the Blueprints and C# scripts for additional logic.

note: This project uses many classes from the base first person demo from unreal engine. Some of these files have been modified. All unique original files can be found in the private and public sub folders.

## Download

You can download the latest build of the portal demo game [here](https://drive.google.com/file/d/1XQGadItVdYzw4-aQqdyhNw70yR_grIRX/view?usp=sharing).

## Development Blog

For a more detailed development blog, visit my portfolio website [here](#) WIP.
