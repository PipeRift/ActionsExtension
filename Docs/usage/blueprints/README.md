# Usage in Blueprints

## Call an Action

To execute an action we have to use the Action node.

You can find it by right clicking on a graph and searching for **"Action"**:

![Add Action](../img/level_blueprint_add_action.png)

Then we assign the action class we want to use.

![Set Action](../img/set_action_type.png)

After this, all its variables and delegates will show up for you to use and the action is ready to execute.

## Create  an Action

Lets start by creating a very simple action.

First we go to the **content browser, right click, Blueprint Class**

![Right Click on content browser](../img/context_blueprint.png)

Then we **select Action class** (or any other child class of Action)

![Select Action class](../img/popup_blueprint.png)

Then we **open the blueprint** we created and add the following functions on Activate. This will be called when the action starts its execution, then wait 1 second, and finish.

![Simple action](../img/simple_action.png)

{% hint style='error' %} Make sure your actions call **Succeed** or **Fail**. Otherwise the action will run until its owner is destroyed or the game closes. {% endhint %}