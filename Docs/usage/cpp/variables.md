# Exposing Variables

Most of the times, when we have an action, we want to feed it with variables to customize its behavior, and as you will see, it's very easy to do so.

Member variables marked as **BlueprintReadWrite** and with metadata **ExposeOnSpawn="true"** will show on Blueprint Action nodes.

![Variable Definition](img/variable_definition.png)

With C++ in particular, any public variable can be edited as usual before Activation.
