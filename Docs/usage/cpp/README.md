# Usage in C++

{% hint style='tip' %} All functions are well commented on code. Feel free to give them a look for detailed information. {% endhint %}

## Call an Action

There are multiple ways of creating an action, here are some:

![Create Action](img/create_action.png)

By default actions will **auto activate**, but if we want to do some kind of setup, we can leave activation for later:

![Custom Activation](img/custom_activation.png)

{% hint style='danger' %} Note that Actions require an owner with access to world. Otherwise, activation will fail. {% endhint %}

## Create  an Action

You can simply create a child class of UAction (or any other action class).

![Action Class](img/action_class.png)

{% hint style='danger' %} Make sure your actions call **Succeed** or **Fail**. Otherwise the action will run until its owner is destroyed or the game closes. {% endhint %}