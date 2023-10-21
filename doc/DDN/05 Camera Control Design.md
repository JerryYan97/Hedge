# Camera control design

In this doc, we write down some built-in camera design for convenience. User should be able to create their own camera entities to achieve the control that they want.

A free-floating camera like Unreal editor.

Right vector needs to be horizontal.
View vectors need to be arbitrary.

## Free floating camera model:

The goal of this camera is to achieve the effect of unreal engine's camera. 

As for translation, this camera entity would listen to W, A, S and D to move along view vector direction or the right vector direction.

As for rotation, this camera entity would listen to the middle mouse click/hold. When we first hold, we record the hold-view, hold-up and hold-right vectors. Besides, we would also record the mouse position on the screen when we first hold.

When we are holding, we constantly update the view and up camera parameters by using the recorded information above. 

The new mouse position's x offset would lead to the horizontal rotation and the y offset would lead to the vertical rotation.

The x offset and the world up vector would be use to calcualte the head rotation matrix (Rotate around the world up vector). The y offset and the hold-right vector would be used to calculate the pitch rotation matrix (Rotate around the hold-right vector).

Then, we multiply the head rotation matrix to the pitch rotation matrix to get rotation matrix. (Matrix are row-major and the head rotation matrix is at the left of the multiplication and the pitch rotation matrix is at the right of the multiplication. So, when we apply that to a vector, we apply pitch rotation first and then apply the head rotation) This rotation matrix would be used to multiply the hold-view and hold-up vector to generate new view and new up vectors to update the camera's view direction (View vector + up vector).

So, when we unhold, the new camera direction would just stay there.