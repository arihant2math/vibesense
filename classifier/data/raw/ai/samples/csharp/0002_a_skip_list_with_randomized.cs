using System;
using System.Collections;
using System.Collections.Generic;

public sealed class SkipList<TKey, TValue> : IEnumerable<KeyValuePair<TKey, TValue>>
{
    private sealed class Node
    {
        public readonly TKey Key;
        public TValue Value;
        public readonly Node[] Forward;

        public Node(TKey key, TValue value, int level)
        {
            Key = key;
            Value = value;
            Forward = new Node[level];
        }
    }

    private readonly object _sync = new object();
    private readonly IComparer<TKey> _comparer;
    private readonly Random _random;
    private readonly double _levelProbability;
    private readonly int _maxLevel;
    private readonly Node _head;
    private int _count;

    public SkipList(
        int maxLevel = 32,
        double levelProbability = 0.5,
        IComparer<TKey> comparer = null,
        int? seed = null)
    {
        if (maxLevel < 1)
            throw new ArgumentOutOfRangeException(nameof(maxLevel), "Maximum level must be at least 1.");

        if (double.IsNaN(levelProbability) ||
            double.IsInfinity(levelProbability) ||
            levelProbability <= 0.0 ||
            levelProbability >= 1.0)
        {
            throw new ArgumentOutOfRangeException(
                nameof(levelProbability),
                "Level probability must be greater than 0 and less than 1.");
        }

        _maxLevel = maxLevel;
        _levelProbability = levelProbability;
        _comparer = comparer ?? Comparer<TKey>.Default;
        _random = seed.HasValue ? new Random(seed.Value) : new Random();
        _head = new Node(default(TKey), default(TValue), maxLevel);
    }

    public int Count
    {
        get
        {
            lock (_sync)
            {
                return _count;
            }
        }
    }

    public void Add(TKey key, TValue value)
    {
        ValidateKey(key);

        lock (_sync)
        {
            Node[] predecessors = FindPredecessors(key);
            Node next = predecessors[0].Forward[0];

            if (next != null && _comparer.Compare(next.Key, key) == 0)
                throw new ArgumentException("An item with the same key already exists.", nameof(key));

            int level = RandomLevel();
            Node node = new Node(key, value, level);

            for (int i = 0; i < level; i++)
            {
                node.Forward[i] = predecessors[i].Forward[i];
                predecessors[i].Forward[i] = node;
            }

            _count++;
        }
    }

    public bool TryAdd(TKey key, TValue value)
    {
        ValidateKey(key);

        lock (_sync)
        {
            Node[] predecessors = FindPredecessors(key);
            Node next = predecessors[0].Forward[0];

            if (next != null && _comparer.Compare(next.Key, key) == 0)
                return false;

            int level = RandomLevel();
            Node node = new Node(key, value, level);

            for (int i = 0; i < level; i++)
            {
                node.Forward[i] = predecessors[i].Forward[i];
                predecessors[i].Forward[i] = node;
            }

            _count++;
            return true;
        }
    }

    public bool ContainsKey(TKey key)
    {
        ValidateKey(key);

        lock (_sync)
        {
            return FindNode(key) != null;
        }
    }

    public bool TryGetValue(TKey key, out TValue value)
    {
        ValidateKey(key);

        lock (_sync)
        {
            Node node = FindNode(key);

            if (node == null)
            {
                value = default(TValue);
                return false;
            }

            value = node.Value;
            return true;
        }
    }

    public TValue GetValueOrThrow(TKey key)
    {
        ValidateKey(key);

        lock (_sync)
        {
            Node node = FindNode(key);

            if (node == null)
                throw new KeyNotFoundException("The specified key was not found.");

            return node.Value;
        }
    }

    public void SetValue(TKey key, TValue value)
    {
        ValidateKey(key);

        lock (_sync)
        {
            Node node = FindNode(key);

            if (node == null)
                throw new KeyNotFoundException("The specified key was not found.");

            node.Value = value;
        }
    }

    public bool Remove(TKey key)
    {
        ValidateKey(key);

        lock (_sync)
        {
            Node[] predecessors = FindPredecessors(key);
            Node target = predecessors[0].Forward[0];

            if (target == null || _comparer.Compare(target.Key, key) != 0)
                return false;

            for (int i = 0; i < _maxLevel; i++)
            {
                if (predecessors[i].Forward[i] != target)
                    break;

                predecessors[i].Forward[i] = target.Forward[i];
            }

            _count--;
            return true;
        }
    }

    public void Clear()
    {
        lock (_sync)
        {
            Array.Clear(_head.Forward, 0, _head.Forward.Length);
            _count = 0;
        }
    }

    public IEnumerator<KeyValuePair<TKey, TValue>> GetEnumerator()
    {
        List<KeyValuePair<TKey, TValue>> snapshot = new List<KeyValuePair<TKey, TValue>>();

        lock (_sync)
        {
            Node current = _head.Forward[0];

            while (current != null)
            {
                snapshot.Add(new KeyValuePair<TKey, TValue>(current.Key, current.Value));
                current = current.Forward[0];
            }
        }

        return snapshot.GetEnumerator();
    }

    IEnumerator IEnumerable.GetEnumerator()
    {
        return GetEnumerator();
    }

    private Node FindNode(TKey key)
    {
        Node current = _head;

        for (int level = _maxLevel - 1; level >= 0; level--)
        {
            while (current.Forward[level] != null &&
                   _comparer.Compare(current.Forward[level].Key, key) < 0)
            {
                current = current.Forward[level];
            }
        }

        Node result = current.Forward[0];
        return result != null && _comparer.Compare(result.Key, key) == 0
            ? result
            : null;
    }

    private Node[] FindPredecessors(TKey key)
    {
        Node[] predecessors = new Node[_maxLevel];
        Node current = _head;

        for (int level = _maxLevel - 1; level >= 0; level--)
        {
            while (current.Forward[level] != null &&
                   _comparer.Compare(current.Forward[level].Key, key) < 0)
            {
                current = current.Forward[level];
            }

            predecessors[level] = current;
        }

        return predecessors;
    }

    private int RandomLevel()
    {
        int level = 1;

        while (level < _maxLevel && _random.NextDouble() < _levelProbability)
            level++;

        return level;
    }

    private static void ValidateKey(TKey key)
    {
        if (ReferenceEquals(key, null))
            throw new ArgumentNullException(nameof(key));
    }
}
