# Asset System

For the Hedge's asset system, it has an asset manager as well as various assets. Assets are large resources that we don't want them to always stay in the RAM. Thus, we use the asset manager to see whether an asset is used by an entity/component in the scene. If there are no entity/component uses this asset, we will release the memory that holds this asset.

Within the game or editor, the asset is referred by its guid, which is generated from the path of this asset.

An asset consists of a yaml file as well as several other files. The yaml file serves as the configuration of the asset and the other files serve as data (e.g. image, obj, binary, ...). The developer can create their own assets and we also provide some basic asset types.

## Static Mesh Asset

The src file must contains positions, uv, normal and tangents.

### Full yaml format

```
asset type: HStaticMeshAsset
asset name: <The name of this asset>
src file: <The mesh file that it uses. E.g. OBJ, glTF, ...>
material: <The name path of the used material>
```

## Material Asset

Currently, we only support the PBR material.

### Full yaml format

```
asset type: HMaterialAsset
asset name: <The name of this asset>
base color: <A pure color or the path name of a texture asset>
albedo texture: <A path to the base color texture. Optional>
normal texture: <A path to the normal texture. Optional. If the asset doesn't have a normal texture, we will use a pure blue texture.>
roughness-metalic texture: <A path to the roughness-metalic texture. Optional. We will use full roughness and no metalic by default.>
occlusion texture: <A path to the occlusion texture. Optional. We don't have any occlusion by default.>
...
```

## Texture Asset

### Full yaml format

```
asset type: HTextureAsset
asset name: <The name of this asset>
...
```
