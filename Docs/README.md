# Actions Extension

**Actions Extension** is a plugin that adds **blueprintable async tasks** called actions. It can be used for a lot of things but some examples are AI or API Rest. 

If you like our plugins, consider becoming a Patron. It will go a long way in helping me create more awesome tech!

[![patron](usage/img/patron_small.png)](https://www.patreon.com/bePatron?u=16503983)

## What is an Action?
![Action Node](usage/img/action_node.png)

**Actions** are quite similar to async task nodes (*like Delays or AIMoves*) in their concept, but have some extra features that make them widely useful.

An Action is a blueprint (or c++ class) that executes inside another object to encapsulate logic.

## Where can I use an Action?
**Any object with world context** can have an action and its usage goes from AI behaviors to API Rest calls.

We have tried both options extensively and the results are a lot more simple than normal code. You get better parallel programming, quality of code. At the end it just becomes easier to deal with complex logic.

This system is also heavily focused on the usage of Actions inside Actions, creating a **tree of dependencies**. This is specially useful for **AI**.



## Quick Start

Check [Quick Start](quick-start.md) to see how to setup and configure the plugin.