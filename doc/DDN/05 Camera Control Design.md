A free-floating camera like Unreal editor.

Right vector needs to be horizontal.
View vectors need to be arbitrary.

## Free - floating camera model:

(1.3): Update Up, hold right and not strict head?

When we first hold, we record the hold view, hold up and hold right vectors.

When we are holding, we constantly update the view and up camera parameters by using the recorded information above. The hold right vector would be used to do the pitch rotation. As for the head rotation, we only use world up vector.

Therefore, we can produce the new rotation matrix by using the mouse inputs, hold right vector and world up vector. Then, we apply this rotation matrix to the hold view and hold up vectors to get the new view and new up vectors, which would be used to update the camera's view and up vectors.

Camera model tries and fails:

(1): Using unchanged up vector (World UP) to calculate the right vector:
World UP would multiply arbitrary view to get the right vector.
It can make sure that the right vector is horizontal, which means camera don't have the roll rotation.

But, in this way, we need to do more, since we use arbitrary vector rotation for pitch and head rotation. It majorly affects the pitch rotation. We use the right vector as the arbitrary rotation axis and if the view vector passes the fixed world up vector, the direction of the right would flip. As a result, this flip of the vector direction would flip the rotation matrix if the input pitch radien is unchanged.

If we can detect this flip, then simply negating the input radien would work. But now I don't have a good idea to detect this flip.

(1.1): World up/ Camera up with minor changes:
I may come up to a flip detection method. When we hold the middle button, we record a hold_Up direction. Then, we can calculate the upper pitch radien and lower pitch radien by using dot and acos.

The goal is to let pitch rotation don't affect the right vector direction.

(1.2): Hold right?
If our goal is to not affect the right vector direction, then why don't we store the hold when we hold the middle button?
First, we rotate the holded up vector using the holded right vector. Then, we use the new up vector for head rotation? --> No... It looks like this would result in the roll rotation.
So, I think we want to rotate along the world up axis.
But if the view exceeds the upper or lower pitch boundary and unhold, the next hold right would be flipped. (The new hold right would be flipped by comparing to the old hold right.)

Maybe we can try to detect this flip? and change the camera rough up direction when the flip is detected.
It looks like our real goal is making pitch offset (Move mouse up) increasing to be pitch up. And making 

After a flipping, how to find when to flip at next hold?

(1.3): Update Up, hold right and not strict head?
we rotate the holded up vector using the holded right vector. Then, we use the new up vector for head rotation.
If we do a strict head rotation, we would have the roll rotation. But if we only do the head rotation by using the world up, then the right vector will always have the y component as 0.f.

Maybe we don't even need to hold the right axis? Because if the right vector is on the y plane at first and we only rotate it along the y world axis then the right vector will remain on the y plane.

No: We need to use the hold right axis. New view should only be calculated from the mouse offsets and hold view, up and right instead of using any intermediate data.

(2): Using the changed up vector to calculate the right vector:
Then, let's try to update the up vector along with the view vector.
Directly updating the up vector along with the view vector by using the same rotation matrix is not enough, since the rotation would let the view vector rotates to an arbitrary direction.

This also means it would rotate the up vector to an arbitrary direction, which introduces roll rotation because the new view and up direction can only promise the perpendicular of these two vectors but they cannot promise that the right vector is horizontal.

Maybe, we can get a new rough right to calculate a new rough up.

After applying the rotation matrix, we have new view, new right and new up. Up and right maybe undesirable. So, we first use the new right to calculate the rough right by zeroing the y of the new right and normalizing it. This rough right is horizontal. Then, we use this rough right to calculate the desired up by crossing product the rough right and view to get the desired up. 

## 